#include <stdlib.h>

/**
 * @file User Interface to HCLIB's Data-Driven Futures
 */

/**
 * @brief ddf is an opaque type
 */
struct ddf_st;

struct ddf_st * ddf_create();

/**
 * @brief Allocate and initialize an array of DDFs.
 * If null_terminated, create nb_ddfs-1 and set the last element to NULL.
 */
struct ddf_st ** ddf_create_n(size_t nb_ddfs, int null_terminated);

void ddf_free(struct ddf_st * ddf);

void * ddf_get(struct ddf_st * ddf);

void ddf_put(struct ddf_st * ddf, void * datum);
