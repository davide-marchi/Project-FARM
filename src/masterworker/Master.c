// Gestione segnali:
//      1. Blocca e ignora tutti i segnali (ereditato dal MasterWorker)

// Inclusione dell'header
#include <Master.h>

// Metodo statico per il cleanup in caso di uscita preventiva
static void cleanupMaster(void *threadpoolPtr);

// Corpo del thread Master
void *Master(void *args)
{

	// Dichiarazione di variabili per gli argomenti inseriti dall'utente
	t_args *a = (t_args *)args;
	int argc = (int)a->argc;
	char **argv = (char **)a->argv;
	int Nthread = (int)a->n;
	int Qlen = (int)a->q;
	int Delay = (int)a->t;
	char *DirectoryName = (char *)a->d;
	threadpool_t *threadpool = NULL;

	// Creazione del threadpool con apposito numero di thread e dimensione della coda
	if ((threadpool = createThreadPool(Nthread, Qlen)) == NULL)
	{
		perror("createThreadPool"); // errore fatale
		pthread_exit(&errno);
	}

	// Installazione della funzione di cleanup per gli elementi allocati sullo heap
	pthread_cleanup_push(cleanupMaster, (void *)&threadpool);

	// Esplorazione tramite funzione ricorsiva della directory passata come argomento tramite -d
	if (DirectoryName != NULL && strnlen(DirectoryName, MAX_PATH_LENGHT + 1) > 0)
	{
		exploreDirectory(DirectoryName, Delay, threadpool);
	}

	// Ciclo per aggiungere alla coda dei task anche i vari file inseriti singolarmente
	struct stat statbuf;
	for (int index = optind; index < argc && getTerminationAtomically(&termination) == 0; index++)
	{

		// Controllo se il file indicato è un file regolare, e in tal caso lo aggiungo alla coda
		if ((stat(argv[index], &statbuf) == 0) && S_ISREG(statbuf.st_mode))
		{
			sleepAndAddToThreadpool(Delay, argv[index], threadpool);
		}
		else
		{
			perror("stat");
		}
	}

	// Rimozione della funzione di cleanup e attesa della terminazione del threadpool
	pthread_cleanup_pop(0);
	destroyThreadPool(threadpool, 0);
	return 0;
}

// Funzione per controllare che non si tratti di "path/."
int isDot(char dir[])
{

	// Controllo dell'ultimo carattere del path passato come argomento
	int len = strnlen(dir, MAX_PATH_LENGHT + 1);
	if ((len > 0 && dir[len - 1] == '.'))
		return 1;
	return 0;
}

// Funzione ricorsiva per espolare ricorsivamente le directory ed inserire i task nel threadpool
void exploreDirectory(char directory[], int delay, threadpool_t *threadpool)
{

	// Controllo che si tratti effettivamente di una directory
	struct stat statbuf;
	if (stat(directory, &statbuf) == -1)
	{
		perror("stat");
		return;
	}

	// Tentativo di apertura della directory
	DIR *dir;
	if ((dir = opendir(directory)) == NULL)
	{
		perror("opendir");
		return;
	}
	else
	{

		// Ciclo fino a che non viene chiesta la terminazione e per ogni file nella directory
		struct dirent *file;
		while (getTerminationAtomically(&termination) == 0 && (errno = 0, file = readdir(dir)) != NULL)
		{

			// Allocazione di variabili per il pathname
			struct stat statbuf;
			char pathname[MAX_PATH_LENGHT + 1];

			// Concatenamenti per assemblare il path relativo di ogni file
			strncpy(pathname, directory, MAX_PATH_LENGHT);
			strncat(pathname, "/", MAX_PATH_LENGHT);
			strncat(pathname, file->d_name, MAX_PATH_LENGHT);

			// Prendo gli attributi del file
			if (stat(pathname, &statbuf) == -1)
			{
				perror("stat");
				return;
			}

			// Controllo se si tratta di una directory esploabile ricorsivamente o di un file da passare al threadpool
			if (S_ISDIR(statbuf.st_mode))
			{
				if (!isDot(pathname))
					exploreDirectory(pathname, delay, threadpool);
			}
			else
			{
				sleepAndAddToThreadpool(delay, pathname, threadpool);
			}
		}

		// Controllo se si sonon verirficati errori e chiusura della directory
		if (errno != 0)
			perror("readdir");
		closedir(dir);
	}
}

// Funzione che attende una certa quantità di tempo e poi procede ad aggiungere il task al threadpool
void sleepAndAddToThreadpool(int delay, char pathname[], threadpool_t *threadpool)
{

	// Sleep di una certa quantità di millisecondi e controllo se nle mentre è stata richiesta la terminazione
	msleep(delay);
	if (getTerminationAtomically(&termination) == 0)
	{

		// Allocazione della memoria necessaria per il path relativo del file
		char *filename = malloc((MAX_PATH_LENGHT + 1) * sizeof(char));
		if (filename == NULL)
		{
			perror("malloc");
		}
		else
		{

			// Scrittura del nome del file nella memoria appena allocata ed inserimento nella coda del threadpool
			strncpy(filename, pathname, MAX_PATH_LENGHT + 1);
			int error;
			if ((error = addToThreadPool(threadpool, Worker, (void *)filename)) != 0)
			{
				free(pathname);
				perror("threadpool");
				pthread_exit(&error);
			}
		}
	}
}

// Funzione di cleanup chiamata solo in caso di errori per assicurarsi di liberare la memoria
static void cleanupMaster(void *threadpoolPtr)
{

	// Stampa di un messagio di errore e terminazione del threadpool
	printf("Problems in Master: Cleanup needed\n");
	fflush(stdout);
	destroyThreadPool(*(threadpool_t **)threadpoolPtr, 0);
}