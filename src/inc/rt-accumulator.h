/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include "accumulator.h"

// Accum base implementation for all type of accumulators
 typedef struct accum_impl_t {
    accum_t base;
    // Close the accumulator, no more puts allowed, wrap-up any work left
    void (*close)(struct accum_impl_t * accum);
    // Transmit the accumulator to another context (finish, async etc..)
    accum_t * (*transmit)(struct accum_impl_t * accum, void * params, int properties);
    void (*destroy)(struct accum_impl_t * accum);
 } accum_impl_t;
