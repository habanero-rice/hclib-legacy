/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

struct _hashtable_entry_struct;

/**
 * @brief hash function to be used for key hashing.
 *
 * @param key         the key to hash
 * @param nbBuckets   number of buckets in the table
 */
typedef int (*hashFct)(void * key, int nbBuckets);

typedef struct _hashtable {
    int nbBuckets;
    struct _hashtable_entry_struct ** table;
} hashtable_t;

void * hashtableConcGet(hashtable_t * hashtable, void * key);
int hashtableConcPut(hashtable_t * hashtable, void * key, void * value);

hashtable_t * newHashtable(int nbBuckets);
void destructHashtable(hashtable_t * hashtable);