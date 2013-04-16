#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// Control debug statements
#define DEBUG_DDF 0

#include "runtime-hclib.h"
#include "rt-ddf.h"
#include "runtime-support.h"

#define EMPTY_DATUM_ERROR_MSG "can not put sentinel value for \"uninitialized\" as a value into DDF"

// For 'headDDTWaitList' when a DDF has been satisfied
#define DDF_SATISFIED NULL

// Default value of a DDF datum
#define UNINITIALIZED_DDF_DATA_PTR NULL

// For waiting frontier (last element of the list)
#define UNINITIALIZED_DDF_WAITLIST_PTR ((struct ddt_st *) -1)

// struct ddf_st is the opaque we expose.
// We define a typedef in this unit for convenience
typedef struct ddf_st {
    void * datum;
    struct ddt_st * headDDTWaitList;
} ddf_t;


/**
 * Associate a DDT to a DDF list.
 */
void ddt_init(ddt_t * ddt, ddf_t ** ddf_list) {
    ddt->waitingFrontier = ddf_list;
    ddt->nextDDTWaitingOnSameDDF = NULL;
}

/**
 * Allocate a DDF and initializes it with default values
 */
ddf_t * ddf_create() {
    ddf_t * ddf = (ddf_t *) malloc(sizeof(ddf_t));
    ddf->datum = NULL;
    ddf->headDDTWaitList = UNINITIALIZED_DDF_WAITLIST_PTR;
    return ddf;
}

/**
 * Allocate 'nb_ddfs' DDFs in contiguous memory.
 * \param null_terminated indicates whether or not the last slot is set to NULL.
 */
ddf_t ** ddf_create_n(size_t nb_ddfs, int null_terminated) {
    ddf_t ** ddfs = (ddf_t **) malloc((sizeof(ddf_t*) * nb_ddfs));
    int i = 0;
    int lg = (null_terminated) ? nb_ddfs-1 : nb_ddfs;
    while(i < lg) {
        ddfs[i] = ddf_create();
        i++;
    }
    if (null_terminated) {
        ddfs[lg] = NULL;
    }
    return ddfs;
}

/**
 * Deallocate a ddf pointer
 */
void ddf_free(ddf_t * ddf) {
    free(ddf);
}

static int register_if_ddf_not_ready(ddt_t * ddt, ddf_t * ddf) {
    int registered = 0;
    ddt_t * headDDTWaitList = (ddt_t *) ddf->headDDTWaitList;
    while (headDDTWaitList != DDF_SATISFIED && !registered) {
        /* headDDTWaitList can not be DDF_SATISFIED in here */
        ddt->nextDDTWaitingOnSameDDF = headDDTWaitList;

        // Try to add the ddt to the ddf's list of ddts waiting.
        registered = __sync_bool_compare_and_swap(&(ddf->headDDTWaitList),
                headDDTWaitList, ddt);

        /* may have failed because either some other task tried to be the head or a put occurred. */
        if (!registered) {
            headDDTWaitList = (ddt_t *) ddf->headDDTWaitList;
            /* if waitListOfDDF was set to DDF_SATISFIED, the loop condition will handle that
             * if another task was added, now try to add in front of that
             */
        }
    }
    if (DEBUG_DDF && registered) {
        printf("ddf: %p registered %p as headDDT of ddf %p\n", ddt, ddf->headDDTWaitList, ddf);
    }
    return registered;
}

/**
 * Runtime interface to DDTs.
 * Returns '1' if all ddf dependencies have been satisfied.
 */
int iterate_ddt_frontier(ddt_t * ddt) {
    ddf_t ** currentDDF = ddt->waitingFrontier;

    if (DEBUG_DDF) {
        printf("ddf: Trying to iterate ddt %p, frontier ddf is %p \n", ddt, currentDDF);
    }
    // Try and register on ddf until one is not ready.
    while ((*currentDDF) && !register_if_ddf_not_ready(ddt, *currentDDF)) {
        ++currentDDF;
    }
    // Here we either:
    // - Have hit a ddf not ready (i.e. no 'put' on this one yet)
    // - iterated over the whole list (i.e. all 'put' must have happened)
    if (DEBUG_DDF) { printf("ddf: iterate result %d\n", (*currentDDF == NULL)); }
    ddt->waitingFrontier = currentDDF;
    return (*currentDDF == NULL);
}

/**
 * Get datum from a DDF.
 * Note: this is concurrent with the 'put' operation.
 */
void * ddf_get(ddf_t * ddf) {
    if (ddf->datum == UNINITIALIZED_DDF_DATA_PTR) {
        return NULL;
    }
    return (void *) ddf->datum;
}

/**
 * Put datum in the DDF.
 * Close down registration of DDTs on this DDF and iterate over the
 * DDF's frontier to try to advance DDTs that were waiting on this DDF.
 */
void ddf_put(ddf_t * ddf, void * datum) {
    assert(datum != UNINITIALIZED_DDF_DATA_PTR && EMPTY_DATUM_ERROR_MSG);
    assert(ddf != NULL && "error: can't DDF is a pointer to NULL");
    //TODO Limitation: not enough to guarantee single assignment
    if (DEBUG_DDF) { printf("ddf: put datum %p\n", ddf->datum); }
    assert(ddf->datum == NULL && "error: violated single assignment property for DDFs");

    volatile ddt_t * headDDTWaitList = NULL;
    ddf->datum = datum;
    headDDTWaitList = ddf->headDDTWaitList;
    if (DEBUG_DDF) { printf("ddf: Retrieve ddf %p head's ddt %p\n", ddf, headDDTWaitList); }
    // Try to cas the wait list to prevent new DDTs from registering on this ddf.
    // When cas successed, new DDTs see the DDF has been satisfied and proceed.
    while (!__sync_bool_compare_and_swap(&(ddf->headDDTWaitList),
            headDDTWaitList, DDF_SATISFIED )) {
        headDDTWaitList = ddf->headDDTWaitList;
    }
    if (DEBUG_DDF) {
        printf("ddf: Put successful on ddf %p set head's ddf to %p\n", ddf, ddf->headDDTWaitList);
    }
    // Now iterate over the list of DDTs, and try to advance their frontier.
    ddt_t * currDDT = (ddt_t *) headDDTWaitList;
    while (currDDT != UNINITIALIZED_DDF_WAITLIST_PTR) {
        if (DEBUG_DDF) { printf("ddf: Trying to iterate frontier of ddt %p\n", currDDT); }
        if (iterate_ddt_frontier(currDDT)) {
            // DDT eligible to scheduling
            async_task_t * async_task = rt_ddt_to_async_task(currDDT);
            if (DEBUG_DDF) { printf("ddf: async_task %p\n", async_task); }
            schedule_async(async_task);
        }
        currDDT = currDDT->nextDDTWaitingOnSameDDF;
    }
}
