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

#ifndef HCLIB_H_
#define HCLIB_H_

/**
 * @file Interface to HCLIB
 */

#include "ddf.h"

void hclib_init(int * argc, char ** argv);

void hclib_finalize();


//
// Async property bits
//

#define NO_PROP 0
#define PHASER_TRANSMIT_ALL ((int) 0x1) /**< To indicate an async must register with all phasers */


//
// Async definition and API
//

/**
 * @brief A pointer to a function to be executed asynchronously
 */
typedef void (*asyncFct_t) (void * arg);

// defined in phased.h
struct _phased_t;

/**
 * @brief An async definition
 * Note: Would have like this to be opaque but
 * that would prevent stack allocation.
 */
typedef struct {
    void * fct_ptr;
    void * arg;
    struct ddf_st ** ddf_list; // Null terminated list
    struct _phased_t * phased_clause;
} async_t;

// NULL clauses
#define NO_DDF NULL
#define NO_PHASER NULL
#define NO_ACCUM NULL

/**
 * @brief: spawn a new async.
 * @param[in] fct_ptr: the function to execute
 * @param[in] arg:             argument to the async
 * @param[in] phased_clause:    The list of DDFs the async awaits
 * @param[in] phased_clause:    Specify which phasers to register on
 * @param[in] property:         Flag to pass information to the runtime
 */
void async(asyncFct_t fct_ptr, void * arg,
           struct ddf_st ** ddf_list, struct _phased_t * phased_clause, int property);

//
// Forasync definition and API
//

typedef int forasync_mode_t;
#define FORASYNC_MODE_RECURSIVE 1
#define FORASYNC_MODE_FLAT 0

typedef void (*forasync1D_Fct_t) (void * arg,int index);
typedef void (*forasync2D_Fct_t) (void * arg,int index_outter,int index_inner);
typedef void (*forasync3D_Fct_t) (void * arg,int index_out,int index_mid,int index_inner);

/**
 * @brief To describe loop domain when spawning forasyncs
 */
typedef struct _loop_domain_t {
    int low;
    int high;
    int stride;
    int tile;
} loop_domain_t;

void forasync(void* forasync_fct, void * argv, struct ddf_st ** ddf_list, struct _phased_t * phased_clause, 
            int dim, loop_domain_t * domain, forasync_mode_t mode);

/**
 * @brief starts a new finish scope
 */
void start_finish();

/**
 * @brief ends the current finish scope
 */
void end_finish();

#endif /* HCLIB_H_ */
