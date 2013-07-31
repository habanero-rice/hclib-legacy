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
#include <assert.h>

// Note: OCR must have been built with ELS support
#include "ocr.h"
#include "ocr-lib.h"
#include "ocr-runtime-itf.h"

#include "runtime-callback.h"
#include "rt-ddf.h"

#ifdef HAVE_PHASER
#include "phaser-api.h"
#endif

// Store async task in ELS @ offset 0
#define ELS_OFFSET_ASYNC 0
#define ELS_OFFSET_PHASER_CTX 1

/**
 * @file OCR-based implementation of HCLIB (implements runtime-support.h)
 */

static async_task_t * root_async_task = NULL;

// guid template for async-edt
static ocrGuid_t asyncTemplateGuid;

typedef struct ocr_finish_t_ {
  finish_t finish;
  volatile ocrGuid_t done_event;
} ocr_finish_t;

// Fwd declaration
ocrGuid_t asyncEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);


#ifdef HAVE_PHASER
// This is to store the phaser context of the master thread
// since it is not an EDT, it doesn't have an ELS.
static phaser_context_t * mainEdtPhaserContext;

// The ocr-based implementation stores the phaser-context in the
// ELS of the currently executing task.
phaser_context_t * get_phaser_context_from_els(bool create) {
    ocrGuid_t edtGuid = currentEdtUserGet();
    ocrGuid_t phaserCtxGuid = NULL_GUID;
    if (edtGuid == NULL) {
        // Handling master thread phaser context
        phaserCtxGuid = ((mainEdtPhaserContext == NULL) ? 
            ((ocrGuid_t) phaser_context_construct()) : mainEdtPhaserContext)
    }
    // For EDTs, we rely on the ELS to store the phaser context
    assert(edtGuid != NULL_GUID);
    phaserCtxGuid = ocrElsUserGet(ELS_OFFSET_PHASER_CTX);    
    if (phaserCtxGuid == NULL_GUID) {
        phaser_context_t * ctx = phaser_context_construct();
        phaserCtxGuid = (ocrGuid_t) ctx;
        ocrElsUserSet(ELS_OFFSET_PHASER_CTX, phaserCtxGuid);        
    }
    return (phaser_context_t *) phaserCtxGuid;
}
#endif


void runtime_init(int * argc, char ** argv) {
    ocrConfig_t ocrConfig;
    ocrParseArgs(*argc, (const char**) argv, &ocrConfig);
    ocrInit(&ocrConfig);
    ocrEdtTemplateCreate(&asyncTemplateGuid, asyncEdt, 1, 0);
    #ifdef HAVE_PHASER
    // setup phaser library
    phaser_lib_setup(get_phaser_context_from_els);
    #endif
}

void runtime_finalize() {
    // This is called only when the main implicit finish is done.
    // At this point it's safe to call ocrFinish and turn the ocr runtime off.
    ocrShutdown();
    ocrFinalize();
}

typedef struct {
    async_task_t task; // the actual task
    ddt_t ddt; // ddt meta-information from hclib
} ocr_ddt_t;

static ocrGuid_t guidify_async(async_task_t * async_task) {
    // The right way to do that would be to use a datablock
    // This should work and save the alloc
    return (ocrGuid_t) async_task;
}

static async_task_t * deguidify_async(ocrGuid_t guid) {
    // The right way to do that would be to use a datablock
    // This should work and save the alloc
    return (async_task_t *) guid;
}

/*
 * @brief Retrieve the async task currently executed from the ELS
 */
async_task_t * get_current_async() {
    ocrGuid_t edtGuid = currentEdtUserGet();
    //If currentEDT is NULL_GUID
    if (edtGuid == NULL_GUID) {
        // This must be the main activity
        return root_async_task;
    }
    ocrGuid_t guid = ocrElsUserGet(ELS_OFFSET_ASYNC);
    return deguidify_async(guid);
}

void set_current_async(async_task_t * async_task) {
    ocrGuid_t edtGuid = currentEdtUserGet ();
    if (edtGuid == NULL_GUID) {
        root_async_task = async_task;
    } else {
        ocrGuid_t guid = guidify_async(async_task);
        ocrElsUserSet(ELS_OFFSET_ASYNC, guid);
    }
}

/**
 * @brief An async execution is backed by an ocr edt
 */
ocrGuid_t asyncEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // The async_task_t data-structure is the single argument.
    async_task_t * async_task = (async_task_t *) paramv;

    // Stores a pointer to the async task in the ELS
    ocrGuid_t guid = guidify_async(async_task);
    ocrElsUserSet(ELS_OFFSET_ASYNC, guid);

    // Call back in the hclib runtime
    rtcb_async_run(async_task);

    return NULL_GUID;
}
/**
 * @brief The HCLIB view of an async task
 * @param def contains data filled in by the user (args, await list, etc.)
 */

async_task_t * rt_ddt_to_async_task(ddt_t * ddt) {
    return (async_task_t *) ((void *)ddt-(void*)sizeof(async_task_t));
}

ddt_t * rt_async_task_to_ddt(async_task_t * async_task) {
    return &(((ocr_ddt_t*) async_task)->ddt);
}

async_task_t * rt_allocate_ddt(struct ddf_st ** ddf_list) {
    ocr_ddt_t * dd_task = (ocr_ddt_t *) calloc(1, sizeof(ocr_ddt_t));
    ddt_init((ddt_t *)&(dd_task->ddt), ddf_list);
    assert(dd_task && "calloc failed");
    return (async_task_t *) dd_task;
}

async_task_t * rt_allocate_async_task() {
    async_task_t * async_task = (async_task_t *) calloc(1, sizeof(async_task_t));
    assert(async_task && "calloc failed");
    return async_task;
}

void rt_schedule_async(async_task_t * async_task) {
    ocrGuid_t edtGuid;
    //Note: Forcefully pass async_task as a (u64 *) to avoid an extra-malloc.
    u8 retCode = ocrEdtCreate(&edtGuid, asyncTemplateGuid,
            EDT_PARAM_DEF, (u64*) async_task, EDT_PARAM_DEF, NULL, 0, NULL_GUID, NULL);
    assert(!retCode);
    // No dependences, the EDT gets scheduled right-away
}

finish_t * rt_allocate_finish() {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) malloc(sizeof(ocr_finish_t));
    assert(ocr_finish && "malloc failed");
    ocr_finish->done_event = NULL_GUID;
    return (finish_t *) ocr_finish;
}

void rt_deallocate_finish(finish_t * finish) {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) finish;
    if (ocr_finish->done_event != NULL_GUID) {
        ocrEventDestroy(ocr_finish->done_event);
    }
    free(ocr_finish);
}

void rt_finish_reached_zero(finish_t * finish) {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) finish;
    if (ocr_finish->done_event != NULL_GUID) {
        ocrEventSatisfy(ocr_finish->done_event, NULL_GUID);
    }
}

void rt_help_finish(finish_t * finish) {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) finish;

    // The event is only required to be able to use ocrWait
    u8 retCode = ocrEventCreate((ocrGuid_t *) &(ocr_finish->done_event), OCR_EVENT_STICKY_T, false);
    assert(!retCode);

    // Here we're racing with other asyncs checking out of the same finish scope
    rtcb_check_out_finish(finish);

    // OCR will try to make progress by executing other EDTs until
    // the event is satisfied (i.e. finish-scope completed)
    ocrWait(ocr_finish->done_event);
}
