#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {

	char fileName[256];
    long result;
	struct node *next;

} Node;

typedef Node *NodePtr;

// Funzione l'inserimento di un nodo in test aalla lista
int insertList(NodePtr *lPtr, long n, char*fileName);

// Funzione per comparare due nodi e poter ordinare i risultato ottenuti
int cmpFunc (const void * a, const void * b);

// Funzione per stampare gli elementi nella lista in modo ordinato (in base al risultato)
int printOrderedList(NodePtr lPtr, size_t size);

// Funzione per liberare tutta la meoria allocata per i nodi presenti nella lista
void freeList(NodePtr *lPtr);

#endif