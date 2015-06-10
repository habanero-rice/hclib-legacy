/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#ifndef RUNTIME_HCLIB_H_
#define RUNTIME_HCLIB_H_

#include "rt-hclib-def.h"

/**
 * @file This file contains the HCLIB runtime implementation
 */


//
// Finish check in/out protocol for asyncs
//

void async_check_in_finish(async_task_t * async_task);
void async_check_out_finish(async_task_t * async_task);

void check_in_finish(finish_t * finish);
void check_out_finish(finish_t * finish);

void async_drop_phasers(async_task_t * async_task);


//
// Current async accessors
//

void set_current_async(async_task_t * async);
async_task_t * get_current_async();


//
// Finish utilities functions
//

finish_t * get_current_finish();
void end_finish_notify(finish_t * current_finish);
void help_finish(finish_t * finish);

//
// Allocators / Deallocators
//

forasync1D_task_t * allocate_forasync1D_task();
forasync2D_task_t * allocate_forasync2D_task();
forasync3D_task_t * allocate_forasync3D_task();
void deallocate_forasync_task(forasync_task_t * forasync);

async_task_t * allocate_async_task(async_t * async_def);
void deallocate_async_task(async_task_t * async);

finish_t * allocate_finish();
void deallocate_finish(finish_t * finish);


//
// Async Scheduling
//

void schedule_async(async_task_t * async_task, finish_t * finish_scope, int property);
void try_schedule_async(async_task_t * async_task);


//
// Workers info
//

int get_nb_workers();
int get_worker_id();

#endif /* RUNTIME_HCLIB_H_ */
