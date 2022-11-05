#ifndef MASTER_H_
#define MASTER_H_

#include <threadpool.h>
#include <MasterWorker.h>
#include <Worker.h>

typedef struct master_args
{
    int argc;
    char **argv;
    int n;
    int q;
    int t;
    char *d;
    
} t_args;

// Corpo del thread Master
void *Master(void *args);

// Funzione per controllare che non si tratti di "path/."
int isDot(char dir[]);

// Funzione ricorsiva per espolare ricorsivamente le directory ed inserire i task nel threadpool
void exploreDirectory(char directory[], int delay, threadpool_t *threadpool);

// Funzione che attende una certa quantità di tempo e poi procede ad aggiungere il task al threadpool
void sleepAndAddToThreadpool(int delay, char pathname[], threadpool_t *threadpool);

#endif
