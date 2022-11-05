// Gestione segnali:
//      1. Blocca e ignora tutti i segnali
//      2. Spawna signal handler che gestisce i vari segnali
//          2.1 SIGNINT -> Far smettere al master di continuare a ciclare per inviare file (variabile globale atomica)
//          2.1 SIGUSR1 -> Invio di -1 al collector e lui capirà che deve stampare
//          2.1 SIGPIPE -> Ignorato
//      3. pthread_cancel del signal handler una volta che non serve più

// Inclusione dell'header
#include <MasterWorker.h>

// Dichiarazione di variabili globali che dovranno essere condivise dai thread del processo
int collectorPid = 0;
long connfd = 0;
pthread_mutex_t termination_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connection_mtx = PTHREAD_MUTEX_INITIALIZER;
volatile int termination = 0;

// Metodo main
int main(int argc, char *argv[])
{

    // Dichiarazione di variabili e assegnamento di valori di default
    int Nthread = 4;
    int Qlen = 8;
    int Delay = 0; // In millisecondi
    char DirectoryName[MAX_PATH_LENGHT + 1];
    DirectoryName[0] = '\0';

    // Blocco dei degnali, i quali dovranno essere gestiti solo dal signal jandler
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask) == -1)
    {
        perror("pthread_sigmask");
        exit(EXIT_FAILURE);
    }

    // Installazione con sigaction per ignorare SIGPIPE
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = SIG_IGN;
    if ((sigaction(SIGPIPE, &s, NULL)) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Controllo che gli argomenti passati siano un numero idoneo
    if (argc < 2)
    {
        printHelper();
        exit(EXIT_FAILURE);
    }

    // Chiamata alla fork e chiamata alla funzione che sarà eseguita dal processo collector
    if ((collectorPid = fork()) == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (collectorPid == 0)
    {
        int collectorReturn = runCollector();
        exit(collectorReturn);
    }

    // Avvio del thread signal handlere gestirà i vari segnali che arrivano al processo
    pthread_t signal_handler_tid;
    if (pthread_create(&signal_handler_tid, NULL, SignalHandler, &mask) != 0)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Creazione del socket AF_UNIX su cui attendere la richiesta di connesione del collector
    int listenfd;
    SYSCALL_EXIT("socket", listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");

    // Preparazione della struttura per descrivere l'indirizzo del socket AF_UNIX (aka AF_LOCAL)
    struct sockaddr_un socketAddr;
    memset(&socketAddr, '0', sizeof(socketAddr));
    socketAddr.sun_family = AF_UNIX;
    strncpy(socketAddr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    // Chiamate bind, listen e accept per aprire effettivamente la connesione col collector
    int notused;
    SYSCALL_EXIT("bind", notused, bind(listenfd, (struct sockaddr *)&socketAddr, sizeof(socketAddr)), "bind", "");
    SYSCALL_EXIT("listen", notused, listen(listenfd, 1), "listen", "");
    SYSCALL_EXIT("accept", connfd, accept(listenfd, (struct sockaddr *)NULL, NULL), "accept", "");
    close(listenfd);

    // Installazione della funzione di cleanup per assicurarsi che le prossime exit chiudano connesione e collector
    if (atexit(cleanupMasterWorker) != 0)
    {
        perror("atexit");
        exit(EXIT_FAILURE); // errore fatale
    }

    // Parsing effettivo degli argomenti passati da tastiera dall'utente
    parseArguments(&Nthread, &Qlen, &Delay, DirectoryName, argc, argv);

    // Riempimento della struttura per passare tutti al thread master
    pthread_t master_tid;
    t_args args;
    args.argc = argc;
    args.argv = argv;
    args.n = Nthread;
    args.q = Qlen;
    args.t = Delay;
    args.d = DirectoryName;

    // Avvio del threaad master
    if (pthread_create(&master_tid, NULL, Master, &args) != 0)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Attesa della terminazione del thread master
    int error;
    if ((error = (pthread_join(master_tid, NULL))) != 0)
    {
        errno = error;
        perror("pthread_join");
    }

    // Terminazione del thread per la gestione dei segnali
    pthread_cancel(signal_handler_tid);
    if ((error = (pthread_join(signal_handler_tid, NULL))) != 0)
    {
        errno = error;
        perror("pthread_join");
    }
    return 0;
}

// Funzione per comunicare all'utente i possibili valori di input
void printHelper()
{

    // Stampa dei vari argomenti possibili e flush dello stdout
    printf("List of the possible arguments:\n");
    printf("  -n <Nthread> Number of worker threads (default = 4)\n");
    printf("  -q <Qlen> Size of the queue of the pending tasks (default = 8)\n");
    printf("  -d <DirectoryName> Name of a directory that needs to be explored\n");
    printf("  -t <Delay> Delay time beetwen two consequntial requests (default = 0)\n");
    fflush(stdout);
}

// Funzione di cleanup per garantire la chiusura della connessione socket e la terminazione del collector
void cleanupMasterWorker()
{

    // Chiusura del file descriptor e unlink per rimuovere il file farm.sck
    close(connfd);
    unlink(SOCKNAME);

    // Attesa della terminazione del Collector a seguito della chiusura della connessione
    int collectorStatus;
    if (waitpid(collectorPid, &collectorStatus, 0) == -1)
    {
        perror("waitpid");
    }
    if (!WIFEXITED(collectorStatus))
    {
        printf("Collector ended with failure (%d)\n", WEXITSTATUS(collectorStatus));
        fflush(stdout);
    }
}

// Funzione per il parsing degli argomenti inseriti da tastiera dall'utente
void parseArguments(int *n, int *q, int *t, char *d, int argc, char *argv[])
{

    // Variabili per sapere quale argomento sia già stato parsato
    long number;
    int opt, nFlag = 0, qFlag = 0, tFlag = 0, dFlag = 0;

    // Ciclo con getopt per il parsing degli argomenti
    while ((opt = getopt(argc, argv, ":n:q:d:t:")) != -1)
    {
        if (optarg != NULL && *optarg == '-')
        {
            optopt = opt;
            opt = ':';
            optind--;
        }
        switch (opt)
        {

        // -n <nthread>  Numero di thread Worker (default = 4)
        case 'n':
            if (nFlag)
            {
                printf("-n can only be used once -> One will be ignored\n");
                break;
            }
            if (isNumber(optarg, &(number)) != 0 || number <= 0)
            {
                printf("-n must be > 0 -> Setted to default value = 4\n");
            }
            else
            {
                *n = (int)number;
            }
            nFlag = 1;
            break;

        // -q <qlen>  Lunghezza della coda concorrente interna (default = 8)
        case 'q':
            if (qFlag)
            {
                printf("-q can only be used once -> One will be ignored\n");
                break;
            }
            if (isNumber(optarg, &(number)) != 0 || number <= 0)
            {
                printf("-q must be > 0 -> Setted to default value = 8\n");
            }
            else
            {
                *q = (int)number;
            }
            qFlag = 1;
            break;

        // -d <directory-name>  Directory in cui sono contenuti file binari ed eventualmente altre directory contenente file binari
        case 'd':
            if (dFlag)
            {
                printf("-d can only be used once -> One will be ignored\n");
                break;
            }
            else
            {
                dFlag = 1;
                strncpy(d, optarg, MAX_PATH_LENGHT + 1);
            }
            break;

        // -t <delay>  Tempo in millisecondi che intercorre tra l’invio di due richieste successive (default = 0)
        case 't':
            if (tFlag)
            {
                printf("-t can only be used once -> One will be ignored\n");
                break;
            }
            if (isNumber(optarg, &(number)) != 0 || number < 0)
            {
                printf("-t must be >= 0 -> Setted to default value = 0\n");
            }
            else
            {
                *t = (int)number;
            }
            tFlag = 1;
            break;

        // Caso in cui manca un argomento
        case ':':
            printf("ERROR: Argument missing\n");
            printHelper();
            exit(EXIT_FAILURE);
            break;

        // Caso in cui non viene riconosciuto un comando
        default:
            printf("ERROR: Unrecognised command\n");
            printHelper();
            exit(EXIT_FAILURE);
            break;
        }
    }
}

// Funzione per avere in modo thread safe una copia della variabile globale per la terminazione preventiva
int getTerminationAtomically(volatile int *termination)
{

    // Accesso alla variabile solamente dop aver acquisito l'apposita lock
    LOCK(&termination_mtx);
    int tmp = *termination;
    UNLOCK(&termination_mtx);
    return tmp;
}

// Thread che gestirà i vari segnali bloccati per gli altri thread del MasterWorker
void *SignalHandler(void *arg)
{

    // Preparazione del set dei segnali da attendere e verificare
    sigset_t *set = (sigset_t *)arg;

    // Ciclo fino a che non viene richiesta la terminazione
    while (getTerminationAtomically(&termination) == 0)
    {
        int sig;
        int r = sigwait(set, &sig);
        if (r != 0)
        {
            errno = r;
            perror("sigwait");
            return NULL;
        }
        switch (sig)
        {

        // Casi in cui il processo deve completare solamente i task eventualmente già presenti nella coda
        case SIGINT:
        case SIGQUIT:
        case SIGHUP:
        case SIGTERM:

            // Assegnemto dell'apposito valori alla variabile
            LOCK(&termination_mtx);
            termination = 1;
            UNLOCK(&termination_mtx);
            break;

        // Caso in cui si deve notificare al Collector di stampare i risultati ricevuti sino a quel momento
        case SIGUSR1:

            // Invio di un codice speciale (-1) che il collector saprà interpretare come richiesta
            LOCK(&connection_mtx);
            int response, specialCode = -1;
            if (writen(connfd, &specialCode, sizeof(int)) > 0)
            {
                if (readn(connfd, &response, sizeof(int)) > 0)
                {
                    if (response == 0)
                    {
                        printf("ERROR: Collector couldn't print the results\n");
                        fflush(stdout);
                    }
                }
            }
            UNLOCK(&connection_mtx);
            break;

        // Caso di default
        default:
            break;
        }
    }
    return NULL;
}