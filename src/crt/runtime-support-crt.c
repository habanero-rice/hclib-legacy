/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "runtime-callback.h"
#include "runtime-support.h"
#include "rt-ddf.h"

#include "hc_sysdep.h"
#include "runtime-crt.h"

#ifdef HAVE_PHASER
#include "phaser-api.h"
#endif

/**
 * @file CRT-based implementation of HCLIB (implements runtime-support.h)
 */

typedef struct crt_finish_t_ {
  finish_t finish;
  volatile short done;
} crt_finish_t;

typedef struct {
    async_task_t task; // the actual task
    ddt_t ddt; // ddt meta-information from hclib
} crt_ddt_t;

#ifdef HAVE_PHASER
phaser_context_t * get_phaser_context_from_els();
#endif


//
// Runtime life-cycle management Implementation
//

void runtime_init(int * argc, char ** argv) {
    // Default options
    int opt_nproc = 4;
    // Primitive arguments checking done here until
    // it's standardized in hclib
    int i;
    char * s;
    for (i = 1; i < *argc; ++i) {
        // Matching potential - and -- options
        if (argv[i][1] == '-') {
            s = argv[i] + 2;
        } else {
            s = argv[i] + 1;
        }
        if (strcmp(s, "nproc") == 0) {
            i++;
            if (i >= (*argc)) {
              printf("error: argument missing for %s\n", s);
              exit(1);
            } 
            opt_nproc = atoi(argv[i]);
        } else {
            // Hit the last HCLIB argument, remove them from argv
            int j;
            for (j = i; j < (*argc); j++) {
                argv[j - (i - 1)] = argv[j];
            }
            *argc -= i - 1;
        }
    }
    
    crt_setup(opt_nproc);
    #ifdef HAVE_PHASER
    // setup phaser library
    phaser_lib_setup(get_phaser_context_from_els);
    #endif
}

void runtime_finalize() {
    crt_shutdown();
}

async_task_t * rt_get_current_async() {
    return (async_task_t *) crt_get_tls_slot(TLS_SLOT_TASK);
}

void rt_set_current_async(async_task_t * async_task) {
    crt_set_tls_slot(TLS_SLOT_TASK, (void *) async_task);
}


//
// Task conversion Implementation
//

async_task_t * rt_ddt_to_async_task(ddt_t * ddt) {
    return (async_task_t *) ((void *)ddt-(void*)sizeof(async_task_t));
}

ddt_t * rt_async_task_to_ddt(async_task_t * async_task) {
    return &(((crt_ddt_t*) async_task)->ddt);
}


//
// Task allocation Implementation
//

async_task_t * rt_allocate_async_task() {
    async_task_t * async_task = (async_task_t *) malloc(sizeof(async_task_t));
    assert(async_task && "malloc failed");
    return async_task;
}

void rt_deallocate_async_task(async_task_t * async_task) {
    free(async_task);
}

async_task_t * rt_allocate_ddt(struct ddf_st ** ddf_list) {
    crt_ddt_t * dd_task = (crt_ddt_t *) malloc(sizeof(crt_ddt_t));
    ddt_init((ddt_t *)&(dd_task->ddt), ddf_list);
    assert(dd_task && "malloc failed");
    return (async_task_t *) dd_task;
}

forasync1D_task_t * rt_allocate_forasync1D_task() {
    forasync1D_task_t * forasync_task = (forasync1D_task_t *) malloc(sizeof(forasync1D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}

forasync2D_task_t * rt_allocate_forasync2D_task() {
    forasync2D_task_t * forasync_task = (forasync2D_task_t *) malloc(sizeof(forasync2D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}

forasync3D_task_t * rt_allocate_forasync3D_task() {
    forasync3D_task_t * forasync_task = (forasync3D_task_t *) malloc(sizeof(forasync3D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}


//
// Finish allocation Implementation
//

finish_t * rt_allocate_finish() {
    crt_finish_t * crt_finish = (crt_finish_t *) malloc(sizeof(crt_finish_t));
    assert(crt_finish && "malloc failed");
    crt_finish->done = 0;
    return (finish_t *) crt_finish;
}

void rt_deallocate_finish(finish_t * finish) {
    free(finish);
}


//
// Scheduling Implementation
//

void rt_schedule_async(async_task_t * async_task) {
    // async is ready for scheduling just push it
    // to the currently executing worker's deque
    int wid = rt_get_worker_id();
    crt_push_work(wid, async_task);
}

void rt_finish_reached_zero(finish_t * finish) {
    crt_finish_t * crt_finish = (crt_finish_t *) finish;
    crt_finish->done = 1;
}

void rt_help_finish(finish_t * finish) {
    // This is called to make progress when an end_finish has been
    // reached but it hasn't completed yet.
    // Note that's also where the master worker ends up entering its work loop

    // Here we're racing with other asyncs checking out of the same finish scope
    rtcb_check_out_finish(finish);
    crt_finish_t * crt_finish = (crt_finish_t *) finish;
    // If finish scope hasn't completed, try to make more progress
    uint64_t wid = (uint64_t) crt_get_tls_slot(TLS_SLOT_WID);
    while(!crt_finish->done) {
        crt_work_shift(wid);
    }
    assert(finish->counter == 0);
}


//
// Worker info Implementation
//

int rt_get_nb_workers() {
    return crt_get_nb_workers();
}

int rt_get_worker_id() {
    return (int) (uint64_t) crt_get_tls_slot(TLS_SLOT_WID);
}

//TODO do we need get_phaser_context_from_els ?
// Defines the function the phaser library should use to
// get the phaser context of the currently executing async
#ifdef HAVE_PHASER
phaser_context_t * get_phaser_context_from_els() {
    async_task_t * current_async = get_current_async();
    // Lazily create phaser contexts
    if (current_async->phaser_context == NULL) {
        current_async->phaser_context = phaser_context_construct();
    }
    return current_async->phaser_context;
}
#endif
