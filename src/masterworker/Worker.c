// Gestione segnali:
//      1. Blocca e ignora tutti i segnali (ereditato dal MasterWorker)

// Inclusione dell'header
#include <Worker.h>

// Funzione eseguita per ogni file passato ai thread del threadpool
void Worker(void *filename)
{

	// Dichiarazione di variabili per il nome del file
	char *file = (char *)filename;
	FILE *fin = NULL;

	// Apertura del file binario come file di input
	if ((fin = fopen(file, "rb")) == NULL)
	{
		perror("fopen");
	}
	else
	{

		// Dichiarazione di variabili per la lettura dal file e ciclo per ogni long che riesco a leggere
		long i = 0, result = 0;
		unsigned char buffer[8];
		while (fread(buffer, sizeof(buffer), 1, fin) == 1)
		{

			// Somma per ottenere il risoltato relativo ad ogni file
			long number = *((long *)buffer);
			result = (i++) * number + result;
		}
		fclose(fin);

		// Lock per evitare che altri Worker usino al connessione
		LOCK(&connection_mtx);
		int len = strnlen(file, MAX_PATH_LENGHT) + 1;
		int notused;

		// Scrittura del numero di caratteri (compreso '\0') che verrà inviato al collector per il nome del file
		SYSCALL_CLEANUP_RETURN("writen WORKER", notused, writen(connfd, &len, sizeof(int)), cleanupWorker(file), "write1\n", "");

		// Scrittura dei caratteri (compreso '\0') del nome del file
		SYSCALL_CLEANUP_RETURN("writen WORKER", notused, writen(connfd, file, len * sizeof(char)), cleanupWorker(file), "write2\n", "");

		// Scrittura del risultato calcolato relativamente al file di cui si è inviato il nome
		SYSCALL_CLEANUP_RETURN("writen WORKER", notused, writen(connfd, &result, sizeof(long)), cleanupWorker(file), "write3\n", "");

		// Lettura della risposta boolena del colector: 1 se è andato tutto a buon fine e 0 altrimenti
		int response = 0;
		SYSCALL_CLEANUP_RETURN("readn WORKER", notused, readn(connfd, &response, sizeof(int)), cleanupWorker(file), "read\n", "");
		if (response == 0)
		{
			printf("Collector couldn't store the message\n");
			fflush(stdout);
		}

		// Rilascio della mutex e liberamento della memoria allocata
		UNLOCK(&connection_mtx);
		free(file);
	}
}

// Funzione di cleanup chiamata solo in caso di errori per assicurarsi di liberare la memoria
void cleanupWorker(char *file)
{

	// Stampa di un messagio di errore, rilacio di lock e richiesta di terminazione al Master tramite termination
	printf("Problems in Worker: Cleanup needed\n");
	fflush(stdout);
	UNLOCK(&connection_mtx);
	free(file);
	LOCK(&termination_mtx);
	termination = 1;
	UNLOCK(&termination_mtx);
}