// Gestione segnali:
//      1. Ignora tutti i segnali (Compreso SIGPIPE) (Gestione e maschera ereditate dalla fork())

// Inclusione dell'header
#include <Collector.h>

// Dichiarazione di variabili globali che dovranno essere accessibili alla funzione di cleanup
int sockfd;
NodePtr listPtr = NULL;

// Funzione per l'esecuzione del Collector
int runCollector()
{

	// Preparazione della struttura per descrivere l'indirizzo del socket AF_UNIX (aka AF_LOCAL)
	struct sockaddr_un socketAddr;
	SYSCALL_EXIT("socket", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");
	memset(&socketAddr, '0', sizeof(socketAddr));
	socketAddr.sun_family = AF_UNIX;
	strncpy(socketAddr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

	// Ciclo fino a che non viene aperta una connessione sul socket
	while (connect(sockfd, (struct sockaddr *)&socketAddr, sizeof(socketAddr)) == -1)
	{

		// Cotrollo se il socket non esiste ancora o non è stato possibile accettare il tentativo di connesione
		if (errno == ENOENT || errno == ECONNREFUSED)
		{

			// Attesa di 200 millisecondi prima di ritentare la connessione
			msleep(200);
		}
		else
		{
			perror("connect");
			exit(EXIT_FAILURE);
		}
	}

	// Installazione della funzione di cleanup per assicurarsi che le prossime exit chiudano connesione e collector
	if (atexit(cleanupCollector) != 0)
	{
		perror("atexit");
		exit(EXIT_FAILURE);
	}

	// Dichiarazione di variabili per la lettura e per contare quanti risultati sono stati ricevuti
	int len, readn_res, n;
	size_t listSize = 0;

	// Lettura delle lunghezza del nome del file che è stato inviato dal MasterWorker
	SYSCALL_EXIT("readn COLLECTOR", readn_res, readn(sockfd, &len, sizeof(int)), "read1\n", "")

	// Ciclo fino a che non leggo EOF oppure leggo una lunghezza non valida
	while (readn_res > 0 && len != 0)
	{

		// Controllo se è stato inviato il numero speciale (-1) per la stampa dei valori ricevuti fino ad ora
		if (len == -1)
		{

			// Stampa dei valori e invio della risposta per segnalare se la stampa è andata a buon fine
			int response = printOrderedList(listPtr, listSize);
			SYSCALL_EXIT("writen COLLECTOR", n, writen(sockfd, &response, sizeof(int)), "write\n", "");
		}
		else
		{

			// Lettura del nome del file e del risultato ad esso associato
			char fileName[256];
			long result;
			SYSCALL_EXIT("readn COLLECTOR", readn_res, readn(sockfd, fileName, len * sizeof(char)), "read2\n", "");
			SYSCALL_EXIT("readn COLLECTOR", readn_res, readn(sockfd, &result, sizeof(long)), "read3\n", "");

			// Inserimento nella lista dei risultati ed invio del risultato booleano dell'inserimento (1 successo, 0 fallimento)
			int response = insertList(&listPtr, result, fileName);
			SYSCALL_EXIT("writen COLLECTOR", n, writen(sockfd, &response, sizeof(int)), "write\n", "");
			listSize = listSize + response;
		}

		// Lettura delle lunghezza del nome del file che è stato inviato dal MasterWorker
		SYSCALL_EXIT("readn COLLECTOR", readn_res, readn(sockfd, &len, sizeof(int)), "read1\n", "")
	}

	// Chiamata alla funzione per la stampa ordinata dei risultati ricevuti
	printOrderedList(listPtr, listSize);
	return 0;
}

// Funzione di cleanup per garantire la chiusura della connessione socket
void cleanupCollector()
{

	// Chiusura del file descriptor, unlink per rimuovere i file farm.sck e liberamneto della memoria usata per la lista
	close(sockfd);
	unlink(SOCKNAME);
	freeList(&listPtr);
}