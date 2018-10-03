
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include "util.h"

#ifndef MULTI_H
#define MULTI_H



typedef struct filestruct{
    FILE *file; // file being worked on
    int numthreads; // number of threads working on this file
    bool done;
   // bool bad; // no longer needed
    pthread_mutex_t fm;
} filewrapper;


typedef struct tosharestruct{
    pthread_mutex_t r; // mutex to protect results file
    FILE *results; // results file
    pthread_mutex_t s; // mutex for serviced.txt
    FILE *serviced; // serviced.txt file

    pthread_mutex_t resolver_log_mutex;
    FILE *resolver_log;
    pthread_mutex_t requester_log_mutex;
    FILE *requester_log;

    bool reqdone; // if true, requester threads are finished
    
    int numfiles; // number of input files
    filewrapper **files; //input files
    
    pthread_cond_t x; //condition variable for resolvers to wait if buffer is empty
    pthread_cond_t y; // condition variable for requester threads to wait if buffer is full
    
    int counter; // number of items in buffer
    pthread_mutex_t c; // mutex to protect counter/buffer
    char **buffer; // buffer to transfer information from requester threads to resolver threads
    
} shared;

int main(int argc, char **argv);

void* resolver(void *input);

void* requester(void *input);

bool req_finished(shared *input);

int select_file(shared *input);

int cleanup(shared *input);

#endif