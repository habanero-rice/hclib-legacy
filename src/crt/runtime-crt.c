/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// Runtime callback API interface to HClib
#include "runtime-crt.h"
#include "runtime-callback.h"
#include "deque.h"

#define CACHE_LINE_L1 8

//
// CRT data-structures
//

// Deques that contain work ready for execution
deque_t ** deques;

// Pthreads backed workers
pthread_t * threads;

// Number of workers
int nb_workers;

typedef struct {
    volatile uint64_t flag;
    void * pad[CACHE_LINE_L1-1];
} worker_done_t;

// Worker loop flags
worker_done_t * allWorkFlag;


//
// CRT implementation of TLS
//

typedef struct {
    void * tls[WORKER_TLS_SIZE];
    //TODO assert not negative
    void * pad[CACHE_LINE_L1-WORKER_TLS_SIZE];
} worker_tls_t;

// TLS storage for all workers
worker_tls_t * allTls;

pthread_key_t selfKey;
pthread_once_t selfKeyInitialized = PTHREAD_ONCE_INIT;

static void initializeKey() {
    pthread_key_create(&selfKey, NULL);
}

void * crt_get_tls_slot(int slot_id) {
    return (((worker_tls_t *) pthread_getspecific(selfKey))->tls[slot_id]);
}

void crt_set_tls_slot(int slot_id, void * arg) {
    worker_tls_t * wtls = (void *) pthread_getspecific(selfKey);
    wtls->tls[slot_id] = arg;
}


//
// CRT runtime information
//

int crt_get_nb_workers() {
    return nb_workers;
}


//
// CRT life-cycle
//

static __inline__ void work_shift(uint64_t wid, deque_t * deque) {
    // try to pop
    void * work = dequePop(deque);
    if (work == NULL) {
        // try to steal
        int i = 1;
        while (i < nb_workers) {
            work = dequeSteal(deques[(wid+i)%(nb_workers)]);
            if (work != NULL) {
                //Tradeoff updating the TLS or in the callback implementation.
                //It's faster here but then every implementation must properly do it.
                allTls[wid].tls[TLS_SLOT_TASK] = work;
                rtcb_async_run((async_task_t *) work);
                break;
            }
            i++;
        }
    } else {
        allTls[wid].tls[TLS_SLOT_TASK] = work;
        rtcb_async_run((async_task_t *) work);
    }
}

static void* worker_routine(void * args) {
    worker_tls_t * wtls = (worker_tls_t *) args;
    pthread_setspecific(selfKey, wtls);
    uint64_t wid = (uint64_t) wtls->tls[TLS_SLOT_WID];
    deque_t * deque = deques[wid];
    while(allWorkFlag[wid].flag) {
        work_shift(wid, deque);
    }
    return NULL;
}

void crt_setup(int nb_workers_) {
    nb_workers = nb_workers_;
    allWorkFlag = malloc(sizeof(worker_done_t) * nb_workers);
    deques = malloc(sizeof(deque_t*) * nb_workers);
    threads = malloc(sizeof(pthread_t) * nb_workers);
    allTls = malloc(sizeof(worker_tls_t) * nb_workers);
    // Setup deques
    uint64_t i;
    for(i=0;i<nb_workers;i++) {
        allWorkFlag[i].flag = 1;
        deques[i] = malloc(sizeof(deque_t));
        dequeInit(deques[i], NULL);
        allTls[i].tls[TLS_SLOT_WID] = (void *) i;
        int slot=1;
        for(;slot<WORKER_TLS_SIZE;slot++) {
            allTls[i].tls[slot] = NULL;
        }
    }
    // Start workers
    pthread_once(&selfKeyInitialized, initializeKey);
    for(i=1;i<nb_workers;i++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&threads[i], &attr, &worker_routine, &allTls[i]);
    }
    // Setting-up current worker (zero) TLS
    pthread_setspecific(selfKey, &allTls[0]);
}

void crt_shutdown() {
    //workers shutdown
    int i;
    for(i=0;i<nb_workers;i++) {
        allWorkFlag[i].flag = 0;
    }
    for(i=1;i< nb_workers; i++) {
        pthread_join(threads[i], NULL);
    }
    for(i=0;i<nb_workers;i++) {
        free(deques[i]);
    }
    free(deques);
    free(threads);
    free(allTls);
}


//
// CRT work-related operations
//

void crt_push_work(int wid, void * work) {
    dequePush(deques[wid], work);
}

void crt_work_shift(int wid) {
    deque_t * deque = deques[wid];
    work_shift(wid, deque);
}
