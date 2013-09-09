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

#include "rt-accumulator.h"
#include "ddf.h"
#include "runtime-hclib.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

//
// Data-structure Types Implementation
//

// Accum Typed implementation
 typedef struct accum_int_t {
    accum_impl_t impl;
    int result;
    void (*open)(struct accum_int_t * accum, int init);
    void (*put)(struct accum_int_t * accum, int v);
    int (*get)(struct accum_int_t * accum);
    void (*op)(int * res, int cur, int new);
 } accum_int_t;

void accum_op_int_plus(int * res, int cur, int new) {
    *res = cur + new;
}

//
// Lazy Implementation
//

typedef struct accum_int_lazy_t {
    accum_int_t base;
    int * localAccums;
} accum_int_lazy_t;


void accum_destroy_int_lazy(accum_impl_t * accum) {
    free(((accum_int_lazy_t *) accum)->localAccums);
    free(accum);
}

void accum_close_int_lazy(accum_impl_t * accum) {
    accum_int_t * accum_int = ((accum_int_t *) accum);
    int * result = &(accum_int->result);
    int * localAccums = ((accum_int_lazy_t *) accum)->localAccums;
    int i = 0;
    int nb_w = get_nb_workers();
    // Sequential reduce of local accumulators
    while (i < nb_w) {
        accum_int->op(result, *result, localAccums[i]);
        i++;
    }
}

accum_t * accum_transmit_int_lazy(accum_impl_t * accum, void * params, int properties) {
    // No-op
    return (accum_t *) accum;
}

void accum_open_int_lazy(accum_int_t * accum, int init) {
    accum->result = init;
    int nb_w = get_nb_workers();
    // Initialize local accumulators for each worker
    accum_int_lazy_t * accum_lazy = (accum_int_lazy_t *) accum;
    int * localAccums = malloc(sizeof(int) * nb_w);
    int i = 0;
    while (i < nb_w) {
        localAccums[i] = init;
        i++;
    }
    accum_lazy->localAccums = localAccums;
}

void accum_put_int_lazy(accum_int_t * accum, int v) {
    int wid = get_worker_id();
    accum_int_lazy_t * accum_lazy = (accum_int_lazy_t *) accum;
    return accum->op(&(accum_lazy->localAccums[wid]), accum_lazy->localAccums[wid], v);
}

int accum_get_int_lazy(accum_int_t * accum) {
    return accum->result;
}

//
// Recursive Implementation
//

typedef struct accum_merge_rec_t {
    accum_t * accum;
    struct ddf_st ** contributors;
} accum_merge_rec_t;

typedef struct accum_int_rec_t {
    accum_int_t base;
    // degree and children could be handled in a latch event and spawn the reduction async
    int degree;
    struct ddf_st ** children;
} accum_int_rec_t;

void accum_destroy_int_rec(accum_impl_t * accum) {
    free(accum);
}

void accum_close_int_rec(accum_impl_t * accum) {
    // No-op
}

// async used in recursive accumulators
void accum_merge_int_rec(void * args) {
    accum_merge_rec_t * reduce = (accum_merge_rec_t *) args;
    accum_int_t * accum = (accum_int_t *) reduce->accum;
    struct ddf_st ** contributors = reduce->contributors;
    int * res = &(accum->result);
    int i = 0;
    while(contributors[i] != NULL) {
        //TODO check where's the storage for int *
        //TODO do we need to free the value pointed by ddf_t ?
        int * contrib = (int *) ddf_get(contributors[i]);
        accum->op(res, *res, *contrib);
        ddf_free(contributors[i]);
        i++;
    }
    free(args);
    // TODO do the put on the parent
}

accum_t * accum_transmit_int_rec(accum_impl_t * accum, void * params, int properties) {
    //TODO double check how this can manage concurrent accesses
    //accum_int_rec_t * accum_rec = (accum_int_rec_t *) accum;
    int * nb_spawn = (int *) params;
    (*nb_spawn)--;
    // Create a new accumulator for the child
    //TODO parameterize init value
    accum_t * accum_child = accum_create_int(ACCUM_OP_NONE, ACCUM_MODE_REC, 0);

    // Setup child's operation
    accum_int_t * accum_int = (accum_int_t *) accum_child;
    accum_int->op = ((accum_int_t *) accum)->op;

    // Setup recursive accumulator members
    accum_int_rec_t * accum_child_rec = (accum_int_rec_t *) accum_child;
    //TODO parameterize degree
    int degree = 2;
    accum_child_rec->degree = degree;

    // Create DDF result place-holder
    struct ddf_st * ddf = ddf_create();
    accum_child_rec->children[*nb_spawn] = ddf;

    if (*nb_spawn == 0) {
        //TODO Add the result bearer ddf/accumulator ?
        accum_merge_rec_t * reducer_args = (accum_merge_rec_t *) malloc(sizeof(accum_merge_rec_t));
        reducer_args->accum = accum_child;
        reducer_args->contributors = (struct ddf_st **) ddf_create_n(degree+1, 1);
        reducer_args->contributors[degree] = NULL;
        //TODO cleanup that 
        async_t * async_def = (async_t *) malloc(sizeof(async_t));
        async(async_def, accum_merge_int_rec, reducer_args,
           reducer_args->contributors, NULL, NO_PROP);
    }

    return accum_child;
}

void accum_open_int_rec(accum_int_t * accum, int init) {
    accum->result = init;
}

void accum_put_int_rec(accum_int_t * accum, int v) {
    return accum->op(&(accum->result), accum->result, v);
}

int accum_get_int_rec(accum_int_t * accum) {
    return accum->result;
}

//
// Sequential Implementation
//

void accum_destroy_int_seq(accum_impl_t * accum) {
    free(accum);
}

void accum_close_int_seq(accum_impl_t * accum) {
    // No-op
}

accum_t * accum_transmit_int_seq(accum_impl_t * accum, void * params, int properties) {
    // No-op
    return (accum_t *) accum;
}

void accum_open_int_seq(accum_int_t * accum, int init) {
    accum->result = init;
}

void accum_put_int_seq(accum_int_t * accum, int v) {
    return accum->op(&(accum->result), accum->result, v);
}

int accum_get_int_seq(accum_int_t * accum) {
    return accum->result;
}

//
// API Implementation
//

accum_t * accum_create_int(accum_op_t op, accum_mode_t mode, int init) {
    accum_int_t * accum_int = NULL;
    if(mode == ACCUM_MODE_SEQ) {
        accum_int = (accum_int_t *) malloc(sizeof(accum_int_t));
        accum_impl_t * accum_impl = (accum_impl_t *) accum_int;
        accum_impl->close = accum_close_int_seq;
        accum_impl->transmit = accum_transmit_int_seq;
        accum_impl->destroy = accum_destroy_int_seq;
        accum_int->open = accum_open_int_seq;
        accum_int->get = accum_get_int_seq;
        accum_int->put = accum_put_int_seq;
    } else if(mode == ACCUM_MODE_REC) {
        accum_int_rec_t * accum_rec = malloc(sizeof(accum_int_rec_t));
        //TODO parameterize degree and contributors
        accum_rec->degree = 2;
        accum_rec->children = NULL;
        accum_int = (accum_int_t *) accum_rec;
        accum_impl_t * accum_impl = (accum_impl_t *) accum_int;
        accum_impl->close = accum_close_int_rec;
        accum_impl->transmit = accum_transmit_int_rec;
        accum_impl->destroy = accum_destroy_int_rec;
        accum_int->open = accum_open_int_rec;
        accum_int->get = accum_get_int_rec;
        accum_int->put = accum_put_int_rec;
    } else if(mode == ACCUM_MODE_LAZY) {
        accum_int_lazy_t * accum_lazy = malloc(sizeof(accum_int_lazy_t));
        //TODO parameterize degree and contributors
        accum_lazy->localAccums = NULL;
        accum_int = (accum_int_t *) accum_lazy;
        accum_impl_t * accum_impl = (accum_impl_t *) accum_int;
        accum_impl->close = accum_close_int_lazy;
        accum_impl->transmit = accum_transmit_int_lazy;
        accum_impl->destroy = accum_destroy_int_lazy;
        accum_int->open = accum_open_int_lazy;
        accum_int->get = accum_get_int_lazy;
        accum_int->put = accum_put_int_lazy;
    } else {
        assert(0 && "error: int accumulator mode not implemented");
    }

    switch(op) {
        case ACCUM_OP_NONE:
            // Used internally
            break;
        case ACCUM_OP_PLUS:
            accum_int->op = accum_op_int_plus;
            break;
        default:
            assert(0 && "error: int accumulator operation not implemented");
    }

    // Open the accumulator
    accum_int->open(accum_int, init);
    return (accum_t *) accum_int;
}

//
// User API Interface Implementation
//

int accum_get_int(accum_t * acc) {
    accum_int_t * acc_int = (accum_int_t *) acc;
    return acc_int->get(acc_int);
}

void accum_put_int(accum_t * acc, int v) {
    accum_int_t * acc_int = (accum_int_t *) acc;
    return acc_int->put(acc_int, v);
}

void accum_register(accum_t ** accs, int n) {
    int i = 0;
    finish_t * finish = get_current_finish();
    //TODO this goes away when we switch to static registration in start_finish
    assert((finish->accumulators == NULL) && "error: overwritting registered accumulators");
    accum_t ** accumulators = (accum_t **) malloc(sizeof(accum_t*) * (n+1));
    while (i < n) {
        accumulators[i] = accs[i];
        i++;
    }
    accumulators[i] = NULL;
    finish->accumulators = accumulators;
}

void accum_destroy(accum_t * acc) {
    accum_impl_t * acc_impl = (accum_impl_t *) acc;
    return acc_impl->destroy(acc_impl);
}
