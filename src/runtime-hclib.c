/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

/**
 * @file This file contains the HCLIB runtime implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hc_sysdep.h"
#include "runtime-support.h"
#include "rt-accumulator.h"
#include "rt-ddf.h"
#ifdef HAVE_PHASER
#include "phased.h"
#endif

// Utils
#include "options.h"


/**
 * @brief Checking in a finish
 */
void check_in_finish(finish_t * finish) {
    hc_atomic32_inc(&(finish->counter));
}

/**
 * @brief Checkout of a finish
 */

void check_out_finish(finish_t * finish) {
    if (hc_atomic32_dec(&(finish->counter))) {
        // If counter reached zero notify runtime
        rt_finish_reached_zero(finish);
    }
}

/**
 * @brief Async task checking in a finish
 */
void async_check_in_finish(async_task_t * async_task) {
    check_in_finish(async_task->current_finish);
}

/**
 * @brief Async task checking out of a finish
 */
void async_check_out_finish(async_task_t * async_task) {
    check_out_finish(async_task->current_finish);
}

/**
 * @brief Drop all phasers an async is registered with and destroy its phaser context
 */
void async_drop_phasers(async_task_t * async_task) {
#ifdef HAVE_PHASER
    if (async_task->phaser_context != NULL) {
        // This call drop all phasers and deallocate the context
        destruct_phaser_context();
        async_task->phaser_context = NULL;
    }
#endif
}

/**
 * @brief Set the currently executing async
 */
void set_current_async(async_task_t * async_task) {
    rt_set_current_async(async_task);
}

/**
 * @brief Returns the currently executing async
 */
async_task_t * get_current_async() {
    return rt_get_current_async();
}


/**
 * @brief Async task allocator. Depending on the nature of the async
 * the runtime may allocate a different data-structure to represent the
 * async. It is the responsibility of the underlying implementation to
 * return a pointer to a valid async_task_t handle.
 */
async_task_t * allocate_async_task(async_t * async_def) {
    async_task_t * async_task;
    if ((async_def != NULL) && async_def->ddf_list != NULL) {
        // When the async has ddfs, we allocate a ddt instead
        // of a regular async. The ddt has extra data-structures.
        async_task = rt_allocate_ddt(async_def->ddf_list);
    } else {
        // Initializes and zeroes
        async_task = rt_allocate_async_task();
    }
    // Copy the definition
    if (async_def != NULL) {
        async_task->def = *async_def;
    }
    #ifdef HAVE_PHASER
    async_task->phaser_context = NULL;
    #endif
    return async_task;
}

/**
 * @brief forasync 1D task allocator. 
 */
forasync1D_task_t * allocate_forasync1D_task() {
    return rt_allocate_forasync1D_task();
}

/**
 * @brief forasync 2D task allocator. 
 */
forasync2D_task_t * allocate_forasync2D_task() {
    return rt_allocate_forasync2D_task();
}

/**
 * @brief forasync 3D task allocator. 
 */
forasync3D_task_t * allocate_forasync3D_task() {
    return rt_allocate_forasync3D_task();
}

/**
 * @brief async task deallocator
 */
void deallocate_async_task(async_task_t * async) {
    rt_deallocate_async_task(async);
}

/**
 * @brief Finish allocator
 */
finish_t * allocate_finish() {
    return rt_allocate_finish();
}

/**
 * @brief Finish deallocator
 */
void deallocate_finish(finish_t * finish) {
    rt_deallocate_finish(finish);
}

/**
 * @brief retrieves the current async's finish scope
 */
finish_t * get_current_finish() {
    // Calling get_current_async is always legal because there's 
    // a fake async to represent the main activity.
    return get_current_async()->current_finish;
}

void end_finish_notify(finish_t * current_finish) {
    accum_t ** accumulators = current_finish->accumulators;
    if(accumulators != NULL) {
        int i = 0;
        while (accumulators[i] != NULL) {
            accum_impl_t * accum_impl = (accum_impl_t *) accumulators[i];
            accum_impl->close(accum_impl);
            i++;
        }
    }
}

static int is_eligible_to_schedule(async_task_t * async_task) {
    if (async_task->def.ddf_list != NULL) {
        ddt_t * ddt = (ddt_t *) rt_async_task_to_ddt(async_task);
        return iterate_ddt_frontier(ddt);
    } else {
        return 1;
    }
}

/**
 * @brief Get the number of workers available to the underlying runtime 
 */
int get_nb_workers() {
    return rt_get_nb_workers();
}

/**
 * @brief Get identifier of currently executing worker
 */
int get_worker_id() {
    return rt_get_worker_id();
}


/**
 * @brief Try scheduling an async that may not be ready for execution
 */
void try_schedule_async(async_task_t * async_task) {
    if (is_eligible_to_schedule(async_task)) {
        rt_schedule_async(async_task);
    }
}

/**
 * @brief Schedule an async that is known for being ready to execute
 */
void schedule_async(async_task_t * async_task, finish_t * finish_scope, int property) {
    // Set the async finish scope to be the currently executing async's one.
    async_task->current_finish = finish_scope;
    // The newly created async must check in the current finish scope
    async_check_in_finish(async_task);

    #ifdef HAVE_PHASER
        phased_t * phased_clause = async_task->def.phased_clause;
        assert(!(phased_clause && (property & PHASER_TRANSMIT_ALL)));
        if (phased_clause != NULL) {
            phaser_context_t * currentCtx = get_phaser_context();
            phaser_context_t * ctx = phaser_context_construct();
            async_task->phaser_context = ctx;
            transmit_specific_phasers(currentCtx, ctx, 
                phased_clause->count, 
                phased_clause->phasers,
                phased_clause->phasers_mode);
        }
        if (property & PHASER_TRANSMIT_ALL) {
            phaser_context_t * currentCtx = get_phaser_context();
            phaser_context_t * ctx = phaser_context_construct();
            async_task->phaser_context = ctx;
            transmit_all_phasers(currentCtx, ctx);
        }
    #endif

    // delegate scheduling to the underlying runtime
    try_schedule_async(async_task);

    // Note: here the async has been scheduled and may or may not
    //       have been executed yet. Careful when adding code here.
}


/**
 * @brief notifies the runtime end_finish is logically
 * blocked, waiting for children to finish.
 */
void help_finish(finish_t * finish) {
    rt_help_finish(finish);
}
