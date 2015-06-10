/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#ifndef DEQUE_H_
#define DEQUE_H_

/****************************************************/
/* DEQUE API                                        */
/****************************************************/

typedef struct {
    volatile int head;
    volatile int tail;
    volatile void ** data;
} deque_t;

void dequeInit(deque_t * deq, void * initValue);
void * dequeSteal(deque_t * deq);
void dequePush(deque_t* deq, void* entry);
void * dequePop(deque_t * deq);
void dequeDestroy(deque_t* deq);

#endif /* DEQUE_H_ */
