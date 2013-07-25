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

#ifndef HCLIB_DEF_H_
#define HCLIB_DEF_H_


/**
 * This file contains runtime-level HCLIB data structures
 */

#include "hclib.h"

#define CHECKED_EXECUTION 0


typedef struct finish {
    volatile int counter;
#if CHECKED_EXECUTION
    int owner; //TODO correctness tracking
#endif
    struct finish * parent;
} finish_t;

struct _async_task_t;

/**
 * @brief Function pointer to an async executor
 */
typedef void (*asyncExecutorFct_t) (struct _async_task_t * async_task);

/**
 * @brief The HCLIB view of an async task
 * @param def contains data filled in by the user (args, await list, etc.)
 */
typedef struct _async_task_t {
    finish_t * current_finish;
    async_t * def;
    asyncExecutorFct_t executor_fct_ptr;
} async_task_t;


#endif /* HCLIB_DEF_H_ */
