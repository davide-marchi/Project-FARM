// Inclusione dell'header
#include <list.h>

// Funzione l'inserimento di un nodo in test aalla lista
int insertList(NodePtr *listPtr, long result, char *fileName)
{

	// Alloco la memoria e controllo che l'operazione dia andata a buon fine
	NodePtr newPtr = malloc(sizeof(Node));
	if (newPtr == NULL)
	{

		// Stampa per segnalare l'errore e ritorno di 0
		printf("Error: Problems during memory allocation\n");
		return 0;
	}

	// Riempimento del nuovo nodo e rictorno di 1
	strncpy(newPtr->fileName, fileName, 256);
	newPtr->result = result;
	newPtr->next = *listPtr;
	*listPtr = newPtr;
	return 1;
}

// Funzione per comparare due nodi e poter ordinare i risultato ottenuti
int cmpFunc(const void *a, const void *b)
{

	// Confronto del valore result dei due nodi e ritrono del risultato
	if ((*(NodePtr *)a)->result > (*(NodePtr *)b)->result)
	{
		return 1;
	}
	return -1;
}

// Funzione per stampare gli elementi nella lista in modo ordinato (in base al risultato)
int printOrderedList(NodePtr listPtr, size_t size)
{

	// Dichiarazione di varaiabili e allocamento della memoria per avere un array
	NodePtr *array = NULL;
	size_t counter = 0;
	if (size < 0 || (array = calloc(size, sizeof(NodePtr))) == NULL)
	{

		// Stampa per segnalare l'errore e ritorno di 0
		printf("Error: Problems during memory allocation\n");
		return 0;
	}

	// Ciclo per ogni nodo presente nella lista dei risultati
	while (listPtr != NULL && counter < size)
	{

		// Inserimento di un puntatore al nodo all'interno dell'array
		array[counter] = listPtr;
		listPtr = listPtr->next;
		counter++;
	}

	// Ordinamento dell'array di puntatori in base al campo result
	qsort(array, counter, sizeof(NodePtr), cmpFunc);

	// Ciclo per ogni elemento nell'array e stampa del contenuto
	for (int i = 0; i < counter; i++)
	{
		printf("%ld %s\n", (array[i])->result, (array[i])->fileName);
	}

	// Liberamento della memoria e ritorno di 1
	free(array);
	return 1;
}

// Funzione per liberare tutta la meoria allocata per i nodi presenti nella lista
void freeList(NodePtr *listPtr)
{

	// Dichiarazione di un puntatore temoraneo e ciclo fino a che non finisco i nodi
	NodePtr tmpPtr = NULL;
	while (*listPtr != NULL)
	{

		// Liberamento della memoria e avanzamento al prossimo nodo
		tmpPtr = *listPtr;
		*listPtr = (*listPtr)->next;
		free(tmpPtr);
	}
}