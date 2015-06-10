/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Note: OCR must have been built with ELS support
#include "ocr.h"
#define ENABLE_EXTENSION_LEGACY 1
#include "extensions/ocr-legacy.h"
#define ENABLE_EXTENSION_RTITF 1
#include "extensions/ocr-runtime-itf.h"

#include "runtime-callback.h"
#include "rt-ddf.h"

#include "hc_sysdep.h"
#include "hashtable.h"

#ifdef HAVE_PHASER
#include "phaser-api.h"
#endif

// Store async task in ELS @ offset 0
#define ELS_OFFSET_ASYNC 0

/**
 * @file OCR-based implementation of HCLIB (implements runtime-support.h)
 */

// Set when initializing hclib
static async_task_t * root_async_task = NULL;
static ocrGuid_t legacyContext = NULL_GUID;

// guid template for async-edt
static ocrGuid_t asyncTemplateGuid;

// To map worker guids to standard ids [0:nbWorkers]
static hashtable_t * guidWidMap = NULL;
static volatile long wid_index = 1;

typedef struct ocr_finish_t_ {
  finish_t finish;
  volatile short done;
} ocr_finish_t;

// Fwd declaration
ocrGuid_t asyncEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);

#ifdef HAVE_PHASER
phaser_context_t * get_phaser_context_from_els();
#endif

// fwd declaration
int rt_get_nb_workers();

void runtime_init(int * argc, char ** argv) {
    ocrConfig_t ocrConfig;
    ocrParseArgs(*argc, (const char**) argv, &ocrConfig);
    ocrLegacyInit(&legacyContext, &ocrConfig);
    ocrEdtTemplateCreate(&asyncTemplateGuid, asyncEdt, 1, 0);
    #ifdef HAVE_PHASER
    // setup phaser library
    phaser_lib_setup(get_phaser_context_from_els);
    #endif
    guidWidMap = newHashtable(rt_get_nb_workers());
}

void runtime_finalize() {
    // This is called only when the main implicit finish is done.
    // At this point it's safe to call ocrShutdown and turn the ocr runtime off.
    ocrShutdown();
    ocrLegacyFinalize(legacyContext, true);
    destructHashtable(guidWidMap);
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
async_task_t * rt_get_current_async() {
    ocrGuid_t edtGuid = currentEdtUserGet();
    //If currentEDT is NULL_GUID
    if (edtGuid == NULL_GUID) {
        // This must be the main activity
        return root_async_task;
    }
    ocrGuid_t guid = ocrElsUserGet(ELS_OFFSET_ASYNC);
    return deguidify_async(guid);
}

void rt_set_current_async(async_task_t * async_task) {
    ocrGuid_t edtGuid = currentEdtUserGet();
    if (edtGuid == NULL_GUID) {
        root_async_task = async_task;
    } else {
        ocrGuid_t guid = guidify_async(async_task);
        ocrElsUserSet(ELS_OFFSET_ASYNC, guid);
    }
}

// Defines the function the phaser library should use to
// get the phaser context of the currently executing async
#ifdef HAVE_PHASER
phaser_context_t * get_phaser_context_from_els() {
    async_task_t * current_async = get_current_async();
    // Lazily create phaser contexts
    if (current_async->phaser_context == NULL) {
        current_async->phaser_context = phaser_context_construct();
    }
    return current_async->phaser_context;
}
#endif

/**
 * @brief An async execution is backed by an ocr edt
 */
ocrGuid_t asyncEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // The async_task_t data-structure is the single argument.
    async_task_t * async_task = (async_task_t *) paramv[0];

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
    ocr_ddt_t * dd_task = (ocr_ddt_t *) malloc(sizeof(ocr_ddt_t));
    ddt_init((ddt_t *)&(dd_task->ddt), ddf_list);
    assert(dd_task && "malloc failed");
    return (async_task_t *) dd_task;
}

async_task_t * rt_allocate_async_task() {
    async_task_t * async_task = (async_task_t *) malloc(sizeof(async_task_t));
    assert(async_task && "malloc failed");
    return async_task;
}

void rt_deallocate_async_task(async_task_t * async_task) {
    free(async_task);
}

forasync1D_task_t * rt_allocate_forasync1D_task() {
    forasync1D_task_t * forasync_task = (forasync1D_task_t *) malloc(sizeof(forasync1D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}

forasync2D_task_t * rt_allocate_forasync2D_task() {
    forasync2D_task_t * forasync_task = (forasync2D_task_t *) malloc(sizeof(forasync2D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}

forasync3D_task_t * rt_allocate_forasync3D_task() {
    forasync3D_task_t * forasync_task = (forasync3D_task_t *) malloc(sizeof(forasync3D_task_t));
    assert(forasync_task && "malloc failed");
    return forasync_task;
}

void rt_schedule_async(async_task_t * async_task) {
    ocrGuid_t edtGuid;
    u64 paramv[1];
    paramv[0] = (u64) async_task;
    u8 retCode = ocrEdtCreate(&edtGuid, asyncTemplateGuid,
        EDT_PARAM_DEF, paramv, EDT_PARAM_DEF, NULL, 0, NULL_GUID, NULL);
    assert(!retCode);
    // No dependences, the EDT gets scheduled right-away
}

finish_t * rt_allocate_finish() {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) malloc(sizeof(ocr_finish_t));
    assert(ocr_finish && "malloc failed");
    ocr_finish->done = 0;
    return (finish_t *) ocr_finish;
}

void rt_deallocate_finish(finish_t * finish) {
    free(finish);
}

void rt_finish_reached_zero(finish_t * finish) {
    ocr_finish_t * ocr_finish = (ocr_finish_t *) finish;
    ocr_finish->done = 1;
}

void rt_help_finish(finish_t * finish) {
    // Here we're racing with other asyncs checking out of the same finish scope
    rtcb_check_out_finish(finish);

    ocr_finish_t * ocr_finish = (ocr_finish_t *) finish;
    // If finish scope hasn't terminated, OCR will try to make progress 
    // by executing another EDT. The loop is exited when the finish-scope has completed.
    while(!ocr_finish->done) {
        ocrInformLegacyCodeBlocking();
    }
    assert(finish->counter == 0);
}

int rt_get_nb_workers() {
    return ocrNbWorkers();
}

// Maintains mapping between worker guids and worker ids
static int find_guid_wid(ocrGuid_t guid) {
    long wid = (long) hashtableConcGet(guidWidMap, (void *) guid);
    if (wid == 0) {
        // Need to insert a new pair
        wid = hc_atomic32_inc(&wid_index);
        hashtableConcPut(guidWidMap, (void *) guid, (void *) wid);
    }
    // -1 so that we're zero based
    return (((int)wid) - 1);
}

int rt_get_worker_id() {
    ocrGuid_t guid = ocrCurrentWorkerGuid();
    return find_guid_wid(guid);
}

