/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include "deque.h"
#include "hc-sysdep.h"

#include <stdlib.h>
#include <assert.h>


#define INIT_DEQUE_CAPACITY 8096


void dequeInit(deque_t * deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **) malloc(sizeof(void*)*INIT_DEQUE_CAPACITY);
    int i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = init_value;
        i++;
    }
}

/*
 * push an entry onto the tail of the deque
 */
void dequePush(deque_t* deq, void* entry) {
    int size = deq->tail - deq->head;
    if (size == INIT_DEQUE_CAPACITY) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        assert("DEQUE full, increase deque's size" && 0);
    }
    int n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;

#ifdef __powerpc64__
    hc_mfence();
#endif
    deq->tail++;
}

void dequeDestroy(deque_t* deq) {
    free(deq->data);
    free(deq);
}

/*
 * the steal protocol
 */
void * dequeSteal(deque_t * deq) {
    int head;
    /* Cannot read deq->data[head] here
     * Can happen that head=tail=0, then the owner of the deq pushes
     * a new task when stealer is here in the code, resulting in head=0, tail=1
     * All other checks down-below will be valid, but the old value of the buffer head
     * would be returned by the steal rather than the new pushed value.
     */
    int tail;
    
    head = deq->head;
    hc_mfence();
    tail = deq->tail;
    if ((tail - head) <= 0) {
        return NULL;
    }

    void * rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];

    /* compete with other thieves and possibly the owner (if the size == 1) */
    if (hc_cas(&deq->head, head, head + 1)) { /* competing */
        return rt;
    }
    return NULL;
}

/*
 * pop the task out of the deque from the tail
 */
void * dequePop(deque_t * deq) {
    hc_mfence();
    int tail = deq->tail;
    tail--;
    deq->tail = tail;
    hc_mfence();
    int head = deq->head;

    int size = tail - head;
    if (size < 0) {
        deq->tail = deq->head;
        return NULL;
    }
    void * rt = (void*) deq->data[(tail) % INIT_DEQUE_CAPACITY];

    if (size > 0) {
        return rt;
    }

    /* now size == 1, I need to compete with the thieves */
    if (!hc_cas(&deq->head, head, head + 1))
        rt = NULL; /* losing in competition */

    /* now the deque is empty */
    deq->tail = deq->head;
    return rt;
}
