/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */
 
#ifndef RT_DDF_H_
#define RT_DDF_H_

#include "rt-hclib-def.h"

/**
 * This file contains the runtime-level DDF interface
 */

// forward declaration
struct ddf_st;

/**
 * DDT data-structure to associate DDTs and DDFs.
 * This is exposed so that the runtime know the size of the struct.
 */
typedef struct ddt_st {
    // NULL-terminated list of DDFs the DDT is registered on
    struct ddf_st ** waitingFrontier;
    // This allows us to chain all DDTs waiting on a same DDF
    // Whenever a DDT wants to register on a DDF, and that DDF is
    // not ready, we chain the current DDT and the DDF's headDDTWaitList
    // and try to cas on the DDF's headDDTWaitList, with the current DDT.
    struct ddt_st * nextDDTWaitingOnSameDDF;
} ddt_t;

/**
 * Initialize a DDT
 */
void ddt_init(ddt_t * ddt, struct ddf_st ** ddf_list);

/**
 * Runtime interface to iterate over the frontier of a DDT.
 * If all DDFs are ready, returns 1, otherwise returns 0
 */
int iterate_ddt_frontier(ddt_t * ddt);

#endif /* RT_DDF_H_ */
