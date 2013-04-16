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
 * DESC: recursive calls with finish (malloc-based)
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hclib.h"

#define NB_ASYNC 127

int * ran = NULL;

void async_fct(int argc, void ** argv) {
    int idx = *((volatile int *) argv);
    assert(ran[idx] == -1);
    ran[idx] = idx;
}

void assert_done(int start, int end) {
    while(start < end) {
        assert(ran[start] == start);
        start++;
    }
}

void init_ran(int *ran, int size) {
    size--;
    while (size >= 0) {
        ran[size] = -1;
        size--;
    }
}

void spawn_async(async_t * async_defs, volatile int * indices, int i) {
    if (i < NB_ASYNC) {
        start_finish();
        indices[i] = i;
        async(async_defs+i, async_fct, 1, (void**) (indices+i), NULL, NULL);
        spawn_async(async_defs, indices, i+1);
        end_finish();
        assert_done(i, i+1);
    }
}

int main (int argc, char ** argv) {
    printf("Call Init\n");
    hclib_init(&argc, argv);
    async_t * asyncs = (async_t *) malloc(sizeof(async_t)*NB_ASYNC);
    volatile int * indices = (int *) malloc(sizeof(int)*NB_ASYNC);
    ran = (int *) malloc(sizeof(int)*NB_ASYNC);
    init_ran(ran, NB_ASYNC);
    start_finish();
    spawn_async(asyncs, indices, 0);
    end_finish();
    printf("Call Finalize\n");
    hclib_finalize();
    printf("Check results: ");
    assert_done(0, NB_ASYNC);
    printf("OK\n");
    return 0;
}
