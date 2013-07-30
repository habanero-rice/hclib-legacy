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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hclib.h"
#include "rt-hclib-def.h"
#include "runtime-support.h"
#include "runtime-hclib.h"

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
void for_wrapper1D(int argc,void **argv,forasync_t *forasynk){

     forasync_ctx ctx= forasynk->ctx;
     int i=0;
     for(i=ctx.low[0];i<ctx.high[0];i++){
	     ((forasyncFct_t1D)(ctx.func))(argc,argv,i);
     }
}
//1D for loop recursive wrapper for forasync
void recursive_wrapper1D(int argc,void **argv,forasync_t *forasynk){

     async_t *async_def=(async_t*)&(forasynk->base);
     forasync_ctx ctx=forasynk->ctx;
     forasync1d_recursive(async_def,ctx.func,ctx.low[0],ctx.high[0], ctx.seq[0], async_def-> argc, async_def->argv,async_def->ddf_list, async_def->phaser_list); 
     
}
/*
//2D for loop wrapper for forasync
void for_wrapper2D(int argc,void **argv,forasync_ctx *ctx){

     int i=0;
     int j=0;
     for(i=ctx->low[0];i<ctx->high[0];i++){
  	   for(j=ctx->low[1];j<ctx->high[1];j++){
		((forasyncFct_t2D)(ctx->func))(argc,argv,i,j);
	   }
    }
}
//3D for loop wrapper for forasync
void for_wrapper3D(int argc,void **argv,forasync_ctx *ctx){

     int i=0;
     int j=0;
     int k=0;
     for(i=ctx->low[0];i<ctx->high[0];i++){
  	   for(j=ctx->low[1];j<ctx->high[1];j++){
		   for(k=ctx->low[2];k<ctx->high[2];k++){
			   ((forasyncFct_t3D)(ctx->func))(argc,argv,i,j,k);
	   }
    }
}*/
//1D chunking for forasync
void forasync1d_chunk(async_t * async_def, void* fct_ptr,int size, int ts, int argc, void ** argv, struct ddf_st ** ddf_list, void * phaser_list) {

    int i=0; 
    int low[3];
    int high[3];
    int seq[3];
    int numblocks=size/ts;
    //number of blocks will depend upon if the ts is a multiple of size or not.
    numblocks = (size-numblocks*ts>0)?numblocks+1:numblocks;

    async_def->fct_ptr = for_wrapper1D;
 
    while (i < numblocks) {

        low[0] = i*ts;
        high[0] = (i*ts+ts)>size?size:(i*ts+ts);
	seq[0] = ts;
        // some middle-impl api
        forasync_task_t *forasync_task = allocate_forasync_task(async_def, low, high,seq,fct_ptr);
	printf("Scheduling Task %d %d\n",forasync_task->def->ctx.low[0],forasync_task->def->ctx.high[0]);
        // Set the async finish scope to be the currently executing async's one.
        forasync_task->current_finish = get_current_async()->current_finish;

        // The newly created async must check in the current finish scope
        async_check_in_finish((async_task_t*)forasync_task);

        // delegate scheduling to the underlying runtime
        schedule_async((async_task_t*)forasync_task);
  	i++;
    }
}
//1D recursive for forasync
void forasync1d_recursive(async_t * async_def, void* fct_ptr,int low,int high, int ts, int argc, void ** argv, struct ddf_st ** ddf_list, void * phaser_list) {

    int lower[3];
    int higher[3];
    int seq[3];

    //split the range into two, spawn a new task for the first half and recurse on the rest  
    if((high-low) > ts) {
       //spawn an async
        lower[0] = (high+low)/2;
        higher[0] = high;
        seq[0] = ts;

        async_def->fct_ptr = recursive_wrapper1D;
        // some middle-impl api
        forasync_task_t *forasync_task = allocate_forasync_task(async_def, lower, higher,seq,fct_ptr);
	printf("Scheduling Task %d %d\n",lower[0],higher[0]);
        // Set the async finish scope to be the currently executing async's one.
        forasync_task->current_finish = get_current_async()->current_finish;

        // The newly created async must check in the current finish scope
        async_check_in_finish((async_task_t*)forasync_task);

        // delegate scheduling to the underlying runtime
        schedule_async((async_task_t*)forasync_task);
        /////////////////////////////////////////////////////////////////////////
        //continue to work on the half task 
	printf("Scheduling Task %d %d\n",low,lower[0]);
	forasync1d_recursive(async_def,fct_ptr,low,lower[0], ts, argc,argv,ddf_list,phaser_list); 

	
    }
    else{//compute the tile
	    int i=0;
	    for(i=low;i<high;i++){
		    ((forasyncFct_t1D)(fct_ptr))(argc,argv,i);
     }

    }
}
//
//  1D version of forasync. runtime_type specifies the type of runtime (1 = recursive) (default = chunk)
//
void forasync1D(async_t* async_def,void* forasync_fct,int size,int ts,int runtime_type, int argc, void ** argv,struct ddf_st ** ddf_list, void * phaser_list) {
    // All the sub-asyncs share async_def
        // Populate the async definition
    async_def->argc = argc;
    async_def->argv = argv;
    async_def->ddf_list = ddf_list;
    async_def->phaser_list = phaser_list;
 
    start_finish();
    if(runtime_type==1){
	forasync1d_recursive(async_def,forasync_fct,0,size, ts, argc,argv,ddf_list,phaser_list); 
    }
    else{
	forasync1d_chunk(async_def,forasync_fct,size,ts,argc,argv,ddf_list,phaser_list);   
    }
    end_finish();
}

void async(async_t * async_def, asyncFct_t fct_ptr, int argc, void ** argv, struct ddf_st ** ddf_list, void * phaser_list) {
    //TODO: api is quite verbose here, the async_def pointer allows
    // users to pass down an address of a stack variable.
#if CHECKED_EXECUTION
    assert(runtime_on);
#endif

    // Populate the async definition
    async_def->fct_ptr = fct_ptr;
    async_def->argc = argc;
    async_def->argv = argv;
    async_def->ddf_list = ddf_list;
    async_def->phaser_list = phaser_list;

    // Build the new async task and associate with the definition
    async_task_t *async_task = allocate_async_task(async_def);

    // Set the async finish scope to be the currently executing async's one.
    async_task->current_finish = get_current_async()->current_finish;

    // The newly created async must check in the current finish scope
    async_check_in_finish(async_task);

    // delegate scheduling to the underlying runtime
    schedule_async(async_task);

    // Note: here the async has been scheduled and may or may not
    //       have been executed yet. Careful when adding code here.
}
