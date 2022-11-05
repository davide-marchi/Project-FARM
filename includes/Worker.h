#ifndef WORKER_H_
#define WORKER_H_

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <MasterWorker.h>

// Funzione eseguita per ogni file passato ai thread del threadpool
void Worker(void *filename);

// Funzione di cleanup chiamata solo in caso di errori per assicurarsi di liberare la memoria
void cleanupWorker(char *args);

#endif