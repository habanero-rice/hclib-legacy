/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include "hashtable.h"

#include <stdlib.h>
#include <stdio.h>

#include "hc_sysdep.h"

/* Hashtable entries */
typedef struct _hashtable_entry_struct {
    void * key;
    void * value;
    struct _hashtable_entry_struct * nxt; /* the next bucket in the hashtable */
} hashtable_entry;

/*
 * Modulo hashing
 */
static int hashModulo(void * ptr, int nbBuckets) {
    return (((unsigned long) ptr) % nbBuckets);
}   

/******************************************************/
/* HASHTABLE COMMON FUNCTIONS                         */
/******************************************************/

/**
 * @brief find hashtable entry for 'key'.
 */
static hashtable_entry* hashtableFindEntry(hashtable_t * hashtable, void * key) {
    int bucket = hashModulo(key, hashtable->nbBuckets);
    hashtable_entry * current = hashtable->table[bucket];
    while(current != NULL) {
        if (current->key == key) {
            return current;
        }
        current = current->nxt;
    }
    return NULL;
}

/**
 * @brief Initialize the hashtable.
 */
static void hashtableInit(hashtable_t * hashtable, int nbBuckets) {
    hashtable_entry ** table = malloc(nbBuckets*sizeof(hashtable_entry*));
    int i;
    for (i=0; i < nbBuckets; i++) {
        table[i] = NULL;
    }
    hashtable->table = table;
    hashtable->nbBuckets = nbBuckets;
}

/**
 * @brief Create a new hashtable instance that uses the specified hashing function.
 */
hashtable_t * newHashtable(int nbBuckets) {
    hashtable_t * hashtable = malloc(sizeof(hashtable_t));
    hashtableInit(hashtable, nbBuckets);
    return hashtable;
}

/**
 * @brief Destruct the hashtable and all its entries (do not deallocate keys and values pointers).
 */
void destructHashtable(hashtable_t * hashtable) {
    // go over each bucket and deallocate entries
    int i = 0;
    while(i < hashtable->nbBuckets) {
        struct _hashtable_entry_struct * bucketHead = hashtable->table[i];
        while (bucketHead != NULL) {
            struct _hashtable_entry_struct * next = bucketHead->nxt;
            free(bucketHead);
            bucketHead = next;
        }
        i++;
    }
    free(hashtable);
}

/******************************************************/
/* CONCURRENT HASHTABLE FUNCTIONS                     */
/******************************************************/

/**
 * @brief get the value associated with a key
 */
void * hashtableConcGet(hashtable_t * hashtable, void * key) {
    hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    return (entry != NULL) ? entry->value : NULL;
}

/**
 * @brief Attempt to insert the key if absent.
 * Return the current value associated with the key
 * if present in the table, otherwise returns the value
 * passed as parameter.
 */
void * hashtableConcTryPut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    int bucket = hashModulo(key, hashtable->nbBuckets);
    hashtable_entry * newHead = NULL;
    int tryPut = 1;
    while(tryPut) {
        // Lookup for the key
        hashtable_entry * oldHead = hashtable->table[bucket];
        // Ensure oldHead is read before iterating
        //TODO not sure which hashtable members should be volatile
        hc_fence();
        hashtable_entry * entry = hashtableFindEntry(hashtable, key);
        if (entry == NULL) {
            // key is not there, try to CAS the head to insert it
            if (newHead == NULL) {
                newHead = malloc(sizeof(hashtable_entry));
                newHead->key = key;
                newHead->value = value;
            }
            newHead->nxt = oldHead;
            // if old value read is different from the old value read by the cmpswap we failed
            tryPut = (((hashtable_entry *) hc_cmpswap64(&hashtable->table[bucket], oldHead, newHead)) != oldHead);
            // If failed, go for another round of check, there's been an insert but we don't
            // know if it's for that key or another one which hash matches the bucket
        } else {
            if (newHead != NULL) {
                // insertion failed because the key had been inserted by a concurrent thread
                free(newHead);
            }
            // key already present, return the value
            return entry->value;
        }
    }
    // could successfully insert the (key,value) pair
    return value;
}

/**
 * @brief Put a key associated with a given value in the map.
 * The put always occurs.
 */
int hashtableConcPut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    int bucket = hashModulo(key, hashtable->nbBuckets);
    hashtable_entry * newHead = malloc(sizeof(hashtable_entry));
    newHead->key = key;
    newHead->value = value;
    int success;
    do {
        hashtable_entry * oldHead = hashtable->table[bucket];
        newHead->nxt = oldHead;
        success = (((hashtable_entry *) hc_cmpswap64(&hashtable->table[bucket], oldHead, newHead)) == oldHead);
    } while (!success);
    return success;
}
