/* Copyright (c) 2013, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Rice University
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/**
 * @file This file contains the HCLIB runtime implementation.
 */

#include <stdlib.h>
#include <assert.h>

#include "hc_sysdep.h"
#include "runtime-support.h"
#include "rt-ddf.h"


/**
 * @brief Checking in a finish
 */
void check_in_finish(finish_t * finish) {
    hc_atomic_inc(&(finish->counter));
}

/**
 * @brief Checkout of a finish
 */
void check_out_finish(finish_t * finish) {
    if (hc_atomic_dec(&(finish->counter))) {
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
    async_task->def = async_def;
    return async_task;
}

/**
 * @brief async task allocator
 */
void deallocate_async_task(async_task_t * async) {
    //TODO rt_deallocate_async_task(async);
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
    // This is legal because there's a fake async
    // to represent the main activity.
    return get_current_async()->current_finish;
}

static int is_eligible_to_schedule(async_task_t * async_task) {
    if (async_task->def->ddf_list != NULL) {
        ddt_t * ddt = (ddt_t *) rt_async_task_to_ddt(async_task);
        return iterate_ddt_frontier(ddt);
    } else {
        return 1;
    }
}

void schedule_async(async_task_t * async_task) {
    if (is_eligible_to_schedule(async_task)) {
        rt_schedule_async(async_task);
    }
}

void help_finish(finish_t * finish) {
    rt_help_finish(finish);
}
