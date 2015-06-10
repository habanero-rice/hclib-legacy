/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

/**
 * @file This file contains the interface to perform call-backs
 * from the runtime implementation to the HCLIB.
 */

#include <stdio.h>

#include "runtime-callback.h"
#include "runtime-hclib.h"

/**
 * @brief Async is starting execution.
 */
inline static void async_run_start(async_task_t * async_task) {
    // Currently nothing to do here

    // Note: not to be mistaken with the 'async'
    //       function that create the async and
    //       check in the finish scope.
}

/**
 * @brief Async is done executing.
 */
inline static void async_run_end(async_task_t * async_task) {
    // Async has ran, checkout from its finish scope
    async_check_out_finish(async_task);
    #ifdef HAVE_PHASER
    async_drop_phasers(async_task);
    #endif
}

/**
 * @brief Call-back to checkout from a finish scope.
 * Note: Just here because of the ocr-based implementation
 */
void rtcb_check_out_finish(finish_t * finish) {
    check_out_finish(finish);
}

/**
 * @brief Call-back for the underlying runtime to run an async.
 */
void rtcb_async_run(async_task_t * async_task) {
    async_run_start(async_task);
    // Call the targeted function with its arguments
    async_t async_def = async_task->def;
    ((asyncFct_t)(async_def.fct_ptr))(async_def.arg);
    async_run_end(async_task);
    deallocate_async_task(async_task);
}
