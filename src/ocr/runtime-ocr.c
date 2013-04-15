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
#include "ocr-runtime-itf.h"

#include "runtime-callback.h"
#include "rt-ddf.h"

// Store async task in ELS @ offset 0
#define ELS_OFFSET 0

/**
 * @file OCR-based implementation of HCLIB (implements runtime-support.h)
 */

static async_task_t * root_async_task = NULL;

void runtime_init(int * argc, char ** argv) {
    ocrInit(argc, argv, 0, NULL);
}

void runtime_finalize() {
    // This is called only when the main implicit finish is done.
    // At this point it's safe to call ocrFinish and turn the ocr runtime off.
    ocrFinish();
    ocrCleanup();
}

typedef struct {
    async_task_t task;
    ocrGuid_t guid;
} ocr_async_task_t;

typedef struct {
    ocr_async_task_t task; // the actual task
    ddt_t ddt; // ddt meta-information from hclib
} ocr_ddt_t;


static ocrGuid_t guidify_async(async_task_t * async_task) {
    ocr_async_task_t * ocr_async_task_wrapper = (ocr_async_task_t *) async_task;
    return (ocrGuid_t) ocr_async_task_wrapper->guid;
}

static async_task_t * deguidify_async(ocrGuid_t guid) {
    async_task_t * task;
    globalGuidProvider->getVal(globalGuidProvider, guid, (u64*)task, NULL);
    return task;
}

/*
 * @brief Retrieve the async task currently executed from the ELS
 */
async_task_t * get_current_async() {
    ocrGuid_t edtGuid = getCurrentEdt();
    if (edtGuid == NULL_GUID) {
        // This must be the main activity
        return root_async_task;
    }
    //If currentEDT is NULL_GUID
    ocrGuid_t guid = ocrElsGet(ELS_OFFSET);
    return deguidify_async(guid);
}

void set_current_async(async_task_t * async_task) {
    ocrGuid_t edtGuid = getCurrentEdt();
    if (edtGuid == NULL_GUID) {
        root_async_task = async_task;
    } else {
        ocrGuid_t guid = guidify_async(async_task);
        ocrElsSet(ELS_OFFSET, guid);
    }
}

/**
 * @brief An async execution is backed by an ocr edt
 */
u8 asyncEdt(u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // The async_task_t data-structure is the single argument.
    async_task_t * async_task = (async_task_t *) paramv;

    // Store a pointer to the async task in the ELS
    ocrGuid_t guid = guidify_async(async_task);
    ocrElsSet(ELS_OFFSET, guid);

    // Call back in the hclib runtime
    async_run(async_task);

    return 0;
}
/**
 * @brief The HCLIB view of an async task
 * @param def contains data filled in by the user (args, await list, etc.)
 */

async_task_t * rt_ddt_to_async_task(ddt_t * ddt) {
    return (async_task_t *) ((void *)ddt-(void*)sizeof(ocr_async_task_t));
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
    async_task_t * async_task = (async_task_t *) calloc(1, sizeof(ocr_async_task_t));
    assert(async_task && "calloc failed");
    return async_task;
}

void rt_schedule_async(async_task_t * async_task) {
    ocr_async_task_t * ocr_async_task_wrapper = (ocr_async_task_t *) async_task;
    //Note: Forcefully pass async_task as a (void **) to avoid an extra-malloc.
    //      To avoid any confusion in the ocr runtime, we state we're not passing
    //      any extra argument because 'paramv' will be copied anyway.
    //      This should work but will surely end-up broken in distributed ocr.
    int retCode;
    retCode = ocrEdtCreate(&(ocr_async_task_wrapper->guid), &asyncEdt,
            0 /*paramc*/, NULL /*params*/, (void**) async_task,
            0 /*properties*/, 0 /*depc*/, NULL /*depv*/);
    assert(!retCode);

    retCode = ocrEdtSchedule((ocrGuid_t)ocr_async_task_wrapper->guid);
    assert(!retCode);
}

