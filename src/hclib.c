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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hclib.h"
#include "rt-hclib-def.h"
#include "runtime-support.h"
#include "runtime-hclib.h"
#ifdef HAVE_PHASER
#include "phased.h"
#include "phaser-api.h"
#endif

/**
 * @file This file should only contain implementations of user-level defined HCLIB functions.
 */

static int runtime_on = 0;
static async_task_t * root_async_task = NULL;

void hclib_init(int * argc, char ** argv) {
    if (runtime_on) {
        assert(0 && "hc_init called twice in a row");
    }

    // Start the underlying runtime
    runtime_init(argc, argv);
    runtime_on = 1;

    // Create the root activity: abstraction to make code more uniform
    root_async_task = allocate_async_task(NULL);
    set_current_async(root_async_task);

    // Create and check in the implicit finish scope.
    start_finish();

    // Go back to user code
}

void hclib_finalize() {
    // Checkout of the implicit finish scope
    if (runtime_on) {
        // Current worker is executing the async root task
        // Check out of the implicit finish scope
        end_finish();
        // Deallocate the root activity
        free(root_async_task);
        // Shutdown the underlying runtime
        runtime_finalize();
        runtime_on = 0;
    } else {
        assert(0 && "hc_finalize called without any matching hc_init");
    }
}

/**
 * @brief start_finish implementation is decoupled in two functions.
 * The main concern is that the implicit finish is executed by the
 * library thread. So we cannot assume it is part of the underlying runtime.
 * Hence, we cannot use get/set current_async. Decoupling allows to feed
 * the function with the root dummy async_task we maintain in a static.
 */
void start_finish() {
    finish_t * finish = allocate_finish();
    assert(finish && "malloc failed");
    async_task_t * async_task = get_current_async();
    finish->counter = 1;
#if CHECKED_EXECUTION
    // The owner of this finish scope is the worker executing this code.
    finish->owner = get_worker_id();
#endif
    // The parent of this finish scope is the finish scope the
    // async is currently executing in.
    finish->parent = async_task->current_finish;
    // The async task has now entered a new finish scope
    async_task->current_finish = finish;
}

void end_finish() {
    //Note: Important to store the current async in a local as the
    //      help-protocol may execute a new async and change the
    //      worker's currently executed async.
    async_task_t * async = get_current_async();
    finish_t * current_finish = async->current_finish;
    // If we still have children executing
    if (current_finish->counter > 1) {
        // Notify the runtime we are waiting for them
        // Note: there's a race on counter. help_finish may
        //       start and realize the finish scope is done.
         help_finish(current_finish);
    }
    // Pop current finish to its parent
    async->current_finish = current_finish->parent;
    // Restore worker's currently executing async
    set_current_async(async);
    // Don't need this finish scope anymore
    deallocate_finish(current_finish);
}
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

//1D chunking for forasync
void forasync_chunk(int type, async_t * async_def, void* fct_ptr,int *higher, int *seq, void * arg, struct ddf_st ** ddf_list, phased_t * phased_clause) {

    int i=0; 
    int j=0; 
    int k=0; 
    int low[3]={0,0,0};
    int high[3]={1,1,1};

    for(i=0; i< higher[2];i+=seq[2]) {
	    low[2] = i;
	    high[2] = (i+seq[2])>higher[2]?higher[2]:(i+seq[2]);
#ifdef HC_LIB_DEBUG
	    printf("Scheduling Task Loop1 %d %d\n",low[2],high[2]);
#endif
	    for(j=0; j< higher[1];j+=seq[1]) {
		    low[1] = j;
		    high[1] = (j+seq[1])>higher[1]?higher[1]:(j+seq[1]);
#ifdef HC_LIB_DEBUG
	            printf("Scheduling Task Loop2 %d %d\n",low[1],high[1]);
#endif
		    for(k=0; k< higher[0];k+=seq[0]) {
			    low[0] = k;
			    high[0] = (k+seq[0])>higher[0]?higher[0]:(k+seq[0]);
			    // some middle-impl api
			    forasync_task_t *forasync_task = allocate_forasync_task(async_def, low, high,seq,fct_ptr);
#ifdef HC_LIB_DEBUG
			    printf("Scheduling Task %d %d\n",forasync_task->def->ctx.low[0],forasync_task->def->ctx.high[0]);
#endif
			    // Set the async finish scope to be the currently executing async's one.
			    forasync_task->current_finish = get_current_async()->current_finish;

			    // The newly created async must check in the current finish scope
			    async_check_in_finish((async_task_t*)forasync_task);

			    // delegate scheduling to the underlying runtime
			    schedule_async((async_task_t*)forasync_task);
		    }
	    }
    }
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
void forasync_recursive1D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list, phased_t * phased_clause) {

    int lower[1]={0};
    int higher[1]={0};
    //split the range into two, spawn a new task for the first half and recurse on the rest  
     if((high[0]-low[0]) > ts[0]) {
	     //spawn an async
	     higher[0] =lower[0] = (high[0]+low[0])/2;
	     // some middle-impl api
	     forasync_task_t *forasync_task = allocate_forasync_task(async_def, lower, high,ts,fct_ptr);
#ifdef HC_LIB_DEBUG
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
#ifdef HC_LIB_DEBUG
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
void forasync_recursive2D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list, phased_t * phased_clause) {

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
#ifdef HC_LIB_DEBUG
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
#ifdef HC_LIB_DEBUG
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
void forasync_recursive3D(async_t * async_def, void* fct_ptr,int *low,int *high, int *ts, void * arg, struct ddf_st ** ddf_list, phased_t * phased_clause) {

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
#ifdef HC_LIB_DEBUG
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
#ifdef HC_LIB_DEBUG
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
void forasync(async_t* async_def, void * arg,struct ddf_st ** ddf_list, phased_t * phased_clause,int dimen,int *size,int *ts,void* forasync_fct,int runtime_type) {
    // All the sub-asyncs share async_def
        // Populate the async definition
    async_def->arg = arg;
    async_def->ddf_list = ddf_list;
    async_def->phased_clause = phased_clause;
 
    int lower[3]={0,0,0};
    int higher[3]={1,1,1};
    int seq[3]={1,1,1};

    start_finish();
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
	    printf("forasync supports upto 3 dimensional loops only.\n");
	    assert(0);
    }
    end_finish();
}

void async(async_t * async_def, asyncFct_t fct_ptr, void * arg,
        struct ddf_st ** ddf_list, struct _phased_t * phased_clause, int property) {
    //TODO: api is quite verbose here, the async_def pointer allows
    // users to pass down an address of a stack variable.
#if CHECKED_EXECUTION
    assert(runtime_on);
#endif

    // Populate the async definition
    async_def->fct_ptr = fct_ptr;
    async_def->arg = arg;
    async_def->ddf_list = ddf_list;
    #ifdef HAVE_PHASER
    async_def->phased_clause = phased_clause;
    #endif
    // Build the new async task and associate with the definition
    async_task_t *async_task = allocate_async_task(async_def);

    // Set the async finish scope to be the currently executing async's one.
    async_task->current_finish = get_current_async()->current_finish;

    // The newly created async must check in the current finish scope
    async_check_in_finish(async_task);

    #ifdef HAVE_PHASER
    assert(!(phased_clause && (property & PHASER_TRANSMIT_ALL)));
    if (phased_clause != NULL) {
        phaser_context_t * currentCtx = get_phaser_context();
        phaser_context_t * ctx = phaser_context_construct();
        async_task->phaser_context = ctx;
        transmit_specific_phasers(currentCtx, ctx, 
            phased_clause->count, 
            phased_clause->phasers,
            phased_clause->phasers_mode);
    }
    if (property & PHASER_TRANSMIT_ALL) {
        phaser_context_t * currentCtx = get_phaser_context();
        phaser_context_t * ctx = phaser_context_construct();
        async_task->phaser_context = ctx;
        transmit_all_phasers(currentCtx, ctx);
    }
    #endif

    // delegate scheduling to the underlying runtime
    schedule_async(async_task);

    // Note: here the async has been scheduled and may or may not
    //       have been executed yet. Careful when adding code here.
}
