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

#include <stdio.h>
#include <assert.h>

#include "hclib.h"
#include "rt-hclib-def.h"
#include "runtime-support.h"
#include "runtime-hclib.h"
#ifdef HAVE_PHASER
#include "phased.h"
#endif

#define DEBUG_FORASYNC 0

void recursive_wrapper1D(void *,forasync_t *);
void recursive_wrapper2D(void *,forasync_t *);
void recursive_wrapper3D(void *,forasync_t *);
void forasync_recursive1D(async_t *, void*,int *low,int *high, int *ts, void * argv, struct ddf_st ** ddf_list, struct _phased_t * phased_clause); 
void forasync_recursive2D(async_t *, void*,int *low,int *high, int *ts, void * argv, struct ddf_st ** ddf_list, struct _phased_t * phased_clause); 
void forasync_recursive3D(async_t *, void*,int *low,int *high, int *ts, void * argv, struct ddf_st ** ddf_list, struct _phased_t * phased_clause); 


//1D for loop wrapper for forasync
void for_wrapper1D(void * arg, forasync_t *forasynk){
     int i=0;
     forasync_ctx ctx= forasynk->ctx;
     for(i=ctx.low[0];i<ctx.high[0];i++){
         ((forasyncFct_t1D)(ctx.func))(arg,i);
     }
}
//2D for loop wrapper for forasync
void for_wrapper2D(void * arg, forasync_t *forasynk){
     int i=0;
     int j=0;
     forasync_ctx ctx= forasynk->ctx;
     for(i=ctx.low[1];i<ctx.high[1];i++){
       for(j=ctx.low[0];j<ctx.high[0];j++){
        ((forasyncFct_t2D)(ctx.func))(arg,i,j);
       }
    }
}
//3D for loop wrapper for forasync
void for_wrapper3D(void * arg, forasync_t *forasynk){
     int i=0;
     int j=0;
     int k=0;
     forasync_ctx ctx= forasynk->ctx;
     for(i=ctx.low[2];i<ctx.high[2];i++){
         for(j=ctx.low[1];j<ctx.high[1];j++){
             for(k=ctx.low[0];k<ctx.high[0];k++){
                 ((forasyncFct_t3D)(ctx.func))(arg,i,j,k);
             }
         }
     }
}

//chunking for forasync
void forasync_chunk(int type, async_t * async_def, void* fct_ptr,int *higher, int *seq, void * arg, struct ddf_st ** ddf_list, struct _phased_t * phased_clause) {
    int i=0;
    int j=0; 
    int k=0; 
    int low[3]={0,0,0};
    int high[3]={1,1,1};
    // Retrieve current finish scope
    finish_t * current_finish = get_current_async()->current_finish;
#if DEBUG_FORASYNC
    int nb_spawn = 0;
#endif
    for(i=0; i< higher[2];i+=seq[2]) {
        low[2] = i;
        high[2] = (i+seq[2])>higher[2]?higher[2]:(i+seq[2]);
#if DEBUG_FORASYNC
        printf("Scheduling Task Loop1 %d %d\n",low[2],high[2]);
#endif
        for(j=0; j< higher[1];j+=seq[1]) {
            low[1] = j;
            high[1] = (j+seq[1])>higher[1]?higher[1]:(j+seq[1]);
#if DEBUG_FORASYNC
            printf("Scheduling Task Loop2 %d %d\n",low[1],high[1]);
#endif
            for(k=0; k< higher[0];k+=seq[0]) {
                low[0] = k;
                high[0] = (k+seq[0])>higher[0]?higher[0]:(k+seq[0]);
                // some middle-impl api
                forasync_task_t *forasync_task = allocate_forasync_task(async_def, low, high,seq,fct_ptr);
#if DEBUG_FORASYNC
                printf("Scheduling Task %d %d\n",forasync_task->def->ctx.low[0],forasync_task->def->ctx.high[0]);
                nb_spawn++;
#endif
                // Set the async finish scope to be the currently executing async's one.
                forasync_task->current_finish = current_finish;

                // The newly created async must check in the current finish scope
                async_check_in_finish((async_task_t*)forasync_task);

                // delegate scheduling to the underlying runtime
                schedule_async((async_task_t*)forasync_task);
            }
        }
    }
#if DEBUG_FORASYNC
    printf("forasync spawned %d\n", nb_spawn);
#endif
}
//3D for loop recursive wrapper for forasync
void recursive_wrapper3D(void * arg, forasync_t *forasynk){

     async_t *async_def=(async_t*)&(forasynk->base);
     forasync_ctx ctx=forasynk->ctx;
     forasync_recursive3D(async_def,ctx.func,ctx.low,ctx.high,ctx.seq, async_def->arg,async_def->ddf_list, async_def->phased_clause); 
     
}
//2D for loop recursive wrapper for forasync
void recursive_wrapper2D(void * arg, forasync_t *forasynk){

     async_t *async_def=(async_t*)&(forasynk->base);
     forasync_ctx ctx=forasynk->ctx;
     forasync_recursive2D(async_def,ctx.func,ctx.low,ctx.high,ctx.seq, async_def->arg,async_def->ddf_list, async_def->phased_clause); 
     
}
//1D for loop recursive wrapper for forasync
void recursive_wrapper1D(void * arg, forasync_t *forasynk){

     async_t *async_def=(async_t*)&(forasynk->base);
     forasync_ctx ctx=forasynk->ctx;
     forasync_recursive1D(async_def,ctx.func,ctx.low,ctx.high,ctx.seq, async_def->arg,async_def->ddf_list, async_def->phased_clause); 
     
}
// recursive1D for forasync
void forasync_recursive1D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list,struct _phased_t * phased_clause) {

    int lower[1]={0};
    int higher[1]={0};
    //split the range into two, spawn a new task for the first half and recurse on the rest  
     if((high[0]-low[0]) > ts[0]) {
         //spawn an async
         higher[0] =lower[0] = (high[0]+low[0])/2;
         // some middle-impl api
         forasync_task_t *forasync_task = allocate_forasync_task(async_def, lower, high,ts,fct_ptr);
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d %d\n",lower[0],high[0]);
#endif
         // Set the async finish scope to be the currently executing async's one.
         forasync_task->current_finish = get_current_async()->current_finish;

         // The newly created async must check in the current finish scope
         async_check_in_finish((async_task_t*)forasync_task);

         // delegate scheduling to the underlying runtime
         schedule_async((async_task_t*)forasync_task);
         /////////////////////////////////////////////////////////////////////////
         //continue to work on the half task 
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d_%d %d_%d\n",low[1],low[0],higher[1],higher[0]);
#endif
         forasync_recursive1D(async_def,fct_ptr,low,higher, ts, arg,ddf_list,phased_clause); 

     }
     else{//compute the tile
         int k=0;
         for(k=low[0];k<high[0];k++){
             ((forasyncFct_t1D)(fct_ptr))(arg,k);
         }
     }
}

// recursive2D for forasync
void forasync_recursive2D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list, struct _phased_t * phased_clause) {

    int lower[2]={0,0};
    int higher[2]={0,0};
    int recurse=0;
    //split the range into two, spawn a new task for the first half and recurse on the rest  
     if((high[1]-low[1]) > ts[1]) {
         higher[1] =lower[1] = (high[1]+low[1])/2;
         lower[0]=low[0];
         higher[0]=high[0];
         recurse=1;
     }
     else  if((high[0]-low[0]) > ts[0]) {
         //spawn an async
         higher[0] =lower[0] = (high[0]+low[0])/2;
         lower[1]=low[1];
         higher[1]=high[1];
         recurse=1;
     }
     if(recurse==1){
         // some middle-impl api
         forasync_task_t *forasync_task = allocate_forasync_task(async_def, lower, high,ts,fct_ptr);
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d_%d %d_%d\n",lower[1],lower[0],high[1],high[0]);
#endif
         // Set the async finish scope to be the currently executing async's one.
         forasync_task->current_finish = get_current_async()->current_finish;

         // The newly created async must check in the current finish scope
         async_check_in_finish((async_task_t*)forasync_task);

         // delegate scheduling to the underlying runtime
         schedule_async((async_task_t*)forasync_task);
         /////////////////////////////////////////////////////////////////////////
         //continue to work on the half task 
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d_%d %d_%d\n",low[1],low[0],higher[1],higher[0]);
#endif
         forasync_recursive2D(async_def,fct_ptr,low,higher, ts, arg,ddf_list,phased_clause); 

     }
     else{//compute the tile
         int j=0;
         int k=0;
         for(j=low[1];j<high[1];j++){
             for(k=low[0];k<high[0];k++){
                 ((forasyncFct_t2D)(fct_ptr))(arg,j,k);
             }
         }
     }
}
// recursive3D for forasync
void forasync_recursive3D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list, struct _phased_t * phased_clause) {

    int lower[3]={0,0,0};
    int higher[3]={0,0,0};
    int recurse=0;
    //split the range into two, spawn a new task for the first half and recurse on the rest  
     if((high[2]-low[2]) > ts[2]) {
         higher[2] =lower[2] = (high[2]+low[2])/2;
         lower[1]=low[1];
         higher[1]=high[1];
         lower[0]=low[0];
         higher[0]=high[0];
         recurse=1;
     }
     else if((high[1]-low[1]) > ts[1]) {
         higher[1] =lower[1] = (high[1]+low[1])/2;
         lower[0]=low[0];
         higher[0]=high[0];
         lower[2]=low[2];
         higher[2]=high[2];
         recurse=1;
     }
     else  if((high[0]-low[0]) > ts[0]) {
         //spawn an async
         higher[0] =lower[0] = (high[0]+low[0])/2;
         lower[2]=low[2];
         higher[2]=high[2];
         lower[1]=low[1];
         higher[1]=high[1];
         recurse=1;
     }
     if(recurse==1){
         // some middle-impl api
         forasync_task_t *forasync_task = allocate_forasync_task(async_def, lower, high,ts,fct_ptr);
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d_%d_%d %d_%d_%d\n",lower[2],lower[1],lower[0],high[2],high[1],high[0]);
#endif
         // Set the async finish scope to be the currently executing async's one.
         forasync_task->current_finish = get_current_async()->current_finish;

         // The newly created async must check in the current finish scope
         async_check_in_finish((async_task_t*)forasync_task);

         // delegate scheduling to the underlying runtime
         schedule_async((async_task_t*)forasync_task);
         /////////////////////////////////////////////////////////////////////////
         //continue to work on the half task 
#if DEBUG_FORASYNC
         printf("Scheduling Tasks %d_%d_%d %d_%d_%d\n",low[2],low[1],low[0],higher[2],higher[1],higher[0]);
#endif
         forasync_recursive3D(async_def,fct_ptr,low,higher, ts, arg,ddf_list,phased_clause); 
     }
     else{//compute the tile
         int i=0;
         int j=0;
         int k=0;
         for(i=low[2];i<high[2];i++){
             for(j=low[1];j<high[1];j++){
                 for(k=low[0];k<high[0];k++){
                     ((forasyncFct_t3D)(fct_ptr))(arg,i,j,k);
                }
            }
         }
     }
}


//
//  forasync. runtime_type specifies the type of runtime (1 = recursive) (default = chunk)
//
void forasync(async_t* async_def, void* forasync_fct, void * arg, struct ddf_st ** ddf_list, 
                    struct _phased_t * phased_clause, struct _accumed_t * accum_clause, 
                    int dimen, int *size, int *ts, int runtime_type) {
    // All the sub-asyncs share async_def
        // Populate the async definition
    async_def->arg = arg;
    async_def->ddf_list = ddf_list;
    async_def->phased_clause = phased_clause;
 
    int lower[3]={0,0,0};
    int higher[3]={1,1,1};
    int seq[3]={1,1,1};

    start_finish();
    if (accum_clause != NULL) {
        accum_register(accum_clause->accums, accum_clause->count);
    }

    if(dimen>0 && dimen<4){
        if(runtime_type==1){
            if(dimen==1){
                async_def->fct_ptr = recursive_wrapper1D;
                higher[0]=size[0];
                seq[0] = ts[0];
                forasync_recursive1D(async_def,forasync_fct,lower,higher, seq, arg,ddf_list,phased_clause); 
            }
            else if(dimen==2){
                async_def->fct_ptr = recursive_wrapper2D;
                higher[0]=size[1];
                seq[0] = ts[1];
                higher[1]=size[0];
                seq[1] = ts[0];
                forasync_recursive2D(async_def,forasync_fct,lower,higher, seq, arg,ddf_list,phased_clause); 
            }
            else if(dimen==3){
                async_def->fct_ptr = recursive_wrapper3D;
                higher[0]=size[2];
                seq[0] = ts[2];
                higher[1]=size[1];
                seq[1] = ts[1];
                higher[2]=size[0];
                seq[2] = ts[0];
                forasync_recursive3D(async_def,forasync_fct,lower,higher, seq, arg,ddf_list,phased_clause); 
            }

        }
        else{
            if(dimen==1){
                async_def->fct_ptr = for_wrapper1D;
                higher[0]=size[0];
                seq[0] = ts[0];
            }
            else if(dimen==2){
                async_def->fct_ptr = for_wrapper2D;
                higher[0]=size[1];
                seq[0] = ts[1];
                higher[1]=size[0];
                seq[1] = ts[0];
            }
            else if(dimen==3){
                async_def->fct_ptr = for_wrapper3D;
                higher[0]=size[2];
                seq[0] = ts[2];
                higher[1]=size[1];
                seq[1] = ts[1];
                higher[2]=size[0];
                seq[2] = ts[0];
            }
            forasync_chunk(dimen,async_def,forasync_fct,higher,seq,arg,ddf_list,phased_clause);   
        }
    }
    else{
        printf("forasync supports up to 3 dimensional loops only.\n");
        assert(0);
    }
    end_finish();
}
