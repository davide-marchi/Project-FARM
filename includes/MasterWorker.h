#ifndef MASTERWORKER_H_
#define MASTERWORKER_H_

#include <conn.h>
#include <util.h>
#include <Master.h>
#include <Collector.h>

#define MAX_PATH_LENGHT 255

extern long connfd;
extern pthread_mutex_t termination_mtx;
extern pthread_mutex_t connection_mtx;
extern volatile int termination;

// Funzione per comunicare all'utente i possibili valori di input
void printHelper();

// Funzione di cleanup per garantire la chiusura della connessione socket e la terminazione del collector
void cleanupMasterWorker();

// Funzione per il parsing degli argomenti inseriti da tastiera dall'utente
void parseArguments(int *n, int *q, int *t, char *d, int argc, char *argv[]);

// Funzione per avere in modo thread safe una copia della variabile globale per la terminazione preventiva
int getTerminationAtomically(volatile int *termination);

// Thread che gestirà i vari segnali bloccati per gli altri thread del MasterWorker
void* SignalHandler(void *arg);

#endif