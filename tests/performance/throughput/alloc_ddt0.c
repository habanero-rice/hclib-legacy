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
 * DESC: Create/Destroy (NB_DDF_DEPS * (TPO * TP_CLASS)) DDFs
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

#include "hclib.h"

static struct timeval t_start;
static struct timeval t_stop;

void start() {
    gettimeofday(&t_start, NULL);
}

// Returns time elapsed in seconds
double stop() {
    gettimeofday(&t_stop, NULL);
    long start = t_start.tv_sec*1000000+t_start.tv_usec;
    long stop = t_stop.tv_sec*1000000+t_stop.tv_usec;
    long elapsed = stop-start;
    double time_sec = ((double)elapsed)/1000000;
    return time_sec;
}

void print_throughput(long nb_instances, double time_sec) {
    //printf("Workload   (unit): %ld\n", nb_instances);
    //printf("Duration   (s)   : %f\n", time_sec);
    printf("Throughput (op/s): %ld\n", (long)(nb_instances/time_sec));
}

#define NB_DDF_DEPS 6
#define TPO 10
#define TP_CLASS 1000000

async_t ** async_store;

int main (int argc, char ** argv) {
    hclib_init(&argc, argv);
    int o = 0;
    int i = 0;
    async_store = (async_t **) malloc(sizeof(async_t *) * TP_CLASS);
    assert(async_store != NULL);
    start();
    while (o < TPO) {
        // allocate a null terminated list of DDFs
        struct ddf_st ** ddf_list = ddf_create_n(NB_DDF_DEPS + 1, 1);
        assert(ddf_list != NULL);
        ddf_list[NB_DDF_DEPS] = NULL;
        i = 0;
        while (i < TP_CLASS) {
            async_t * async_def = (async_t *) malloc(sizeof(async_t));
            async_def->ddf_list = ddf_list;
            assert(async_def != NULL);
            async_store[i] = async_def;
            i++;
        }

        // deallocate
        i = 0;
        while (i < TP_CLASS) {
            free(async_store[i]);
            i++;
        }
        o++;
    }

    double time_sec = stop();
    print_throughput(TP_CLASS*TPO, time_sec);

    free(async_store);

    hclib_finalize();
    return 0;
}
