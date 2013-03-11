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
 * @file This file contains the HCLIB runtime implementation
 */

#include <stdlib.h>
#include <assert.h>

#include "hc_sysdep.h"
#include "runtime-support.h"

/**
 * @brief Async task checking in a finish
 */
void async_check_in_finish(async_task_t * async_task) {
    finish_t * finish = async_task->current_finish;
    hc_atomic_inc(&(finish->counter));
}

/**
 * @brief Async task checking out of a finish
 */
void async_check_out_finish(async_task_t * async_task) {
    finish_t * finish = async_task->current_finish;
    hc_atomic_dec(&(finish->counter));
}

/**
 * @brief Async task allocator.
 */
async_task_t * allocate_async_task() {
    // Initializes and zeroes
    async_task_t * async_task = (async_task_t *) calloc(1, sizeof(async_task_t));
    assert(async_task && "calloc failed");
    return async_task;
}

/**
 * @brief retrieves the current async's finish scope
 */
finish_t * get_current_finish() {
    // This is legal because there's a fake async
    // to represent the main activity.
    return get_current_async()->current_finish;
}
