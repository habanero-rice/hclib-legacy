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
 * DESC: Fork a bunch of asyncs in a top-level loop
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hclib.h"

#define H1 1024
#define H2 512
#define T1 33
#define T2 217


//user written code
void forasync_fct2(void * argv,int idx1,int idx2) {
    
    int *ran=(int *)argv;
    //printf("%d_%d_%d ",idx1,idx2,ran[idx1*32+idx2]);
    assert(ran[idx1*H2+idx2] == -1);
    ran[idx1*H2+idx2] = idx1*H2+idx2;
}
void init_ran(int *ran, int size) {
    while (size > 0) {
        ran[size-1] = -1;
        size--;
    }
}
void init_check(int *ran, int size) {
    while (size > 0) {
    	assert(ran[size-1] == -1);
        size--;
    }
}
int main (int argc, char ** argv) {
    printf("Call Init\n");
    hclib_init(&argc, argv);
    int i = 0;
    int *ran=(int *)malloc(H1*H2*sizeof(int));
    // This is ok to have these on stack because this
    // code is alive until the end of the program.

    init_ran(ran, H1*H2);
//    init_check(ran, H1*H2*H3);
        //Note: Forcefully pass the address we want to write to as a void **
     async_t forasyn[1];
    int size[2]={H1,H2};
    int seq[2]={T1,T2};
    forasync(forasyn,forasync_fct2,(void*)(ran),NULL, NULL,2,size,seq,1);

    printf("Call Finalize\n");
    hclib_finalize();
    printf("Check results: ");
    i=0;
    while(i < H1*H2) {
        assert(ran[i] == i);
        i++;
    }
    printf("OK\n");
    return 0;
}
