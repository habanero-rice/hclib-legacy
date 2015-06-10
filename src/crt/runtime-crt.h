/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

//
// CRT implementation of TLS
//

#define WORKER_TLS_SIZE 2
#define TLS_SLOT_WID 0
#define TLS_SLOT_TASK 1

void * crt_get_tls_slot(int slot_id);

void crt_set_tls_slot(int slot_id, void * arg);


//
// CRT runtime information
//

int crt_get_nb_workers();


//
// CRT life-cycle
//

void crt_setup(int nb_workers);

void crt_shutdown();


//
// CRT work-related operations
//

void crt_push_work(int wid, void * work);

void crt_work_shift(int wid);
