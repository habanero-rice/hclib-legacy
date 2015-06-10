/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

typedef struct hclib_options {
    int nproc; /* number of physical processors */
    int * bind_map; /* thread binding map */
    int bind_map_size; /* thread binding map size */
} hclib_options;

void read_options(hclib_options * options, int *argc, char *argv[]);
