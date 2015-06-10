/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#ifndef HC_SYSDEP_H_
#define HC_SYSDEP_H_

//
// amd_64
//

#ifdef __x86_64

#define HC_CACHE_LINE 64


#define hc_xadd32(atomic, addValue)                                    \
    ({                                                                  \
        int __tmp = __sync_fetch_and_add(atomic, addValue);             \
        __tmp;                                                          \
    })


#define hc_atomic32_inc(atomic) hc_xadd32(atomic, 1)

// #define hc_atomic32_dec(atomic) hc_xadd32(atomic, -1) == 1

/* return 1 if the *ptr becomes 0 after decremented, otherwise return 0
 */
static __inline__ int hc_atomic32_dec(volatile int *ptr) {
    unsigned char rt;
    __asm__ __volatile__(
            "lock;\n"
            "decl %0; sete %1"
            : "+m" (*(ptr)), "=qm" (rt)
              : : "memory"
    );
    return rt != 0;
}

#define hc_cmpswap64(atomic, cmpValue, newValue)                \
        __sync_val_compare_and_swap(atomic, cmpValue, newValue)

#define hc_fence() \
    do { __sync_synchronize(); } while(0);


#endif /* __x86_64 */

#endif /* HC_SYSDEP_H_ */
