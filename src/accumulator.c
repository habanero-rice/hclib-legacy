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

void accum_op_int_max(int * res, int cur, int new) {
    if(new > cur) {
        *res = new;
    }
}

 typedef struct accum_double_t {
    accum_impl_t impl;
    double result;
    void (*open)(struct accum_double_t * accum, double init);
    void (*put)(struct accum_double_t * accum, double v);
    double (*get)(struct accum_double_t * accum);
    void (*op)(double * res, double cur, double new);
 } accum_double_t;

void accum_op_double_plus(double * res, double cur, double new) {
    *res = cur + new;
}

void accum_op_double_max(double * res, double cur, double new) {
    if(new > cur) {
        *res = new;
    }
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
    int wid = get_worker_id_hc();
    accum_int_lazy_t * accum_lazy = (accum_int_lazy_t *) accum;
    return accum->op(&(accum_lazy->localAccums[wid]), accum_lazy->localAccums[wid], v);
}

int accum_get_int_lazy(accum_int_t * accum) {
    return accum->result;
}

typedef struct accum_double_lazy_t {
    accum_double_t base;
    double * localAccums;
} accum_double_lazy_t;


void accum_destroy_double_lazy(accum_impl_t * accum) {
    free(((accum_double_lazy_t *) accum)->localAccums);
    free(accum);
}

void accum_close_double_lazy(accum_impl_t * accum) {
    accum_double_t * accum_double = ((accum_double_t *) accum);
    double * result = &(accum_double->result);
    double * localAccums = ((accum_double_lazy_t *) accum)->localAccums;
    int i = 0;
    int nb_w = get_nb_workers();
    // Sequential reduce of local accumulators
    while (i < nb_w) {
        accum_double->op(result, *result, localAccums[i]);
        i++;
    }
}

accum_t * accum_transmit_double_lazy(accum_impl_t * accum, void * params, int properties) {
    // No-op
    return (accum_t *) accum;
}

void accum_open_double_lazy(accum_double_t * accum, double init) {
    accum->result = init;
    int nb_w = get_nb_workers();
    // Initialize local accumulators for each worker
    accum_double_lazy_t * accum_lazy = (accum_double_lazy_t *) accum;
    double * localAccums = malloc(sizeof(double) * nb_w);
    int i = 0;
    while (i < nb_w) {
        localAccums[i] = init;
        i++;
    }
    accum_lazy->localAccums = localAccums;
}

void accum_put_double_lazy(accum_double_t * accum, double v) {
    int wid = get_worker_id_hc();
    accum_double_lazy_t * accum_lazy = (accum_double_lazy_t *) accum;
    return accum->op(&(accum_lazy->localAccums[wid]), accum_lazy->localAccums[wid], v);
}

double accum_get_double_lazy(accum_double_t * accum) {
    return accum->result;
}
//
// API Implementation
//
accum_t * accum_create_int(accum_op_t op, accum_mode_t mode, int init) {
    accum_int_t * accum_int = NULL;
    if(mode == ACCUM_MODE_SEQ) {
        assert(0 && "error: int accumulator mode not implemented");
        // accum_int = (accum_int_t *) malloc(sizeof(accum_int_t));
        // accum_impl_t * accum_impl = (accum_impl_t *) accum_int;
        // accum_impl->close = accum_close_int_seq;
        // accum_impl->transmit = accum_transmit_int_seq;
        // accum_impl->destroy = accum_destroy_int_seq;
        // accum_int->open = accum_open_int_seq;
        // accum_int->get = accum_get_int_seq;
        // accum_int->put = accum_put_int_seq;
    } else if(mode == ACCUM_MODE_REC) {
        assert(0 && "error: int accumulator mode not implemented");
        // accum_int_rec_t * accum_rec = malloc(sizeof(accum_int_rec_t));
        // //TODO parameterize degree and contributors
        // accum_rec->degree = 2;
        // accum_rec->children = NULL;
        // accum_int = (accum_int_t *) accum_rec;
        // accum_impl_t * accum_impl = (accum_impl_t *) accum_int;
        // accum_impl->close = accum_close_int_rec;
        // accum_impl->transmit = accum_transmit_int_rec;
        // accum_impl->destroy = accum_destroy_int_rec;
        // accum_int->open = accum_open_int_rec;
        // accum_int->get = accum_get_int_rec;
        // accum_int->put = accum_put_int_rec;
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
        case ACCUM_OP_MAX:
            accum_int->op = accum_op_int_max;
            break;
        default:
            assert(0 && "error: int accumulator operation not implemented");
    }

    // Open the accumulator
    accum_int->open(accum_int, init);
    return (accum_t *) accum_int;
}
accum_t * accum_create_double(accum_op_t op, accum_mode_t mode, double init) {
    accum_double_t * accum_double = NULL;
    if(mode == ACCUM_MODE_SEQ) {
        assert(0 && "error: int accumulator mode not implemented");
        // accum_double = (accum_double_t *) malloc(sizeof(accum_double_t));
        // accum_impl_t * accum_impl = (accum_impl_t *) accum_double;
        // accum_impl->close = accum_close_double_seq;
        // accum_impl->transmit = accum_transmit_double_seq;
        // accum_impl->destroy = accum_destroy_double_seq;
        // accum_double->open = accum_open_double_seq;
        // accum_double->get = accum_get_double_seq;
        // accum_double->put = accum_put_double_seq;
    } else if(mode == ACCUM_MODE_REC) {
        assert(0 && "error: int accumulator mode not implemented");
        // accum_double_rec_t * accum_rec = malloc(sizeof(accum_double_rec_t));
        // //TODO parameterize degree and contributors
        // accum_rec->degree = 2;
        // accum_rec->children = NULL;
        // accum_double = (accum_double_t *) accum_rec;
        // accum_impl_t * accum_impl = (accum_impl_t *) accum_double;
        // accum_impl->close = accum_close_double_rec;
        // accum_impl->transmit = accum_transmit_double_rec;
        // accum_impl->destroy = accum_destroy_double_rec;
        // accum_double->open = accum_open_double_rec;
        // accum_double->get = accum_get_double_rec;
        // accum_double->put = accum_put_double_rec;
    } else if(mode == ACCUM_MODE_LAZY) {
        accum_double_lazy_t * accum_lazy = malloc(sizeof(accum_double_lazy_t));
        //TODO parameterize degree and contributors
        accum_lazy->localAccums = NULL;
        accum_double = (accum_double_t *) accum_lazy;
        accum_impl_t * accum_impl = (accum_impl_t *) accum_double;
        accum_impl->close = accum_close_double_lazy;
        accum_impl->transmit = accum_transmit_double_lazy;
        accum_impl->destroy = accum_destroy_double_lazy;
        accum_double->open = accum_open_double_lazy;
        accum_double->get = accum_get_double_lazy;
        accum_double->put = accum_put_double_lazy;
    } else {
        assert(0 && "error: double accumulator mode not implemented");
    }

    switch(op) {
        case ACCUM_OP_NONE:
            // Used internally
            break;
        case ACCUM_OP_PLUS:
            accum_double->op = accum_op_double_plus;
            break;
        case ACCUM_OP_MAX:
            accum_double->op = accum_op_double_max;
            break;
        default:
            assert(0 && "error: double accumulator operation not implemented");
    }

    // Open the accumulator
    accum_double->open(accum_double, init);
    return (accum_t *) accum_double;
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

double accum_get_double(accum_t * acc) {
    accum_double_t * acc_double = (accum_double_t *) acc;
    return acc_double->get(acc_double);
}

void accum_put_double(accum_t * acc, double v) {
    accum_double_t * acc_double = (accum_double_t *) acc;
    return acc_double->put(acc_double, v);
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
