/* Copyright (c) 2013, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Rice University
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
