#ifndef COLLECTOR_H_
#define COLLECTOR_H_

#include <util.h>
#include <conn.h>
#include <list.h>

// Funzione per l'esecuzione del Collector
int runCollector();

// Funzione di cleanup per garantire la chiusura della connessione socket
void cleanupCollector();

#endif