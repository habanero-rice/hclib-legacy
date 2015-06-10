/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#ifndef RUNTIME_CALLBACK_H_
#define RUNTIME_CALLBACK_H_

/**
 * @file This file contains the interface to perform call-backs
 * from the runtime implementation to the HCLIB.
 */

#include "rt-hclib-def.h"

/**
 * @brief Call back for the runtime implementation to execute an async
 * \param async The async task to execute.
 */
void rtcb_async_run(async_task_t * async);

/**
 * @brief Call back for the runtime implementation to check out of a finish
 * \param async The finish to checkout from.
 */
void rtcb_check_out_finish(finish_t * finish);

#endif /* RUNTIME_CALLBACK_H_ */
