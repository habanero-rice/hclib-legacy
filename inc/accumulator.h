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

#ifndef ACCUMULATOR_H_
#define ACCUMULATOR_H_

typedef enum {
    ACCUM_OP_NONE,
    ACCUM_OP_PLUS,
    ACCUM_OP_MAX
} accum_op_t;

typedef enum {
    ACCUM_MODE_SEQ,
    ACCUM_MODE_LAZY,
    ACCUM_MODE_REC
} accum_mode_t;

typedef struct accum_t {

} accum_t;

typedef struct _accumed_t {
    int count;
    accum_t ** accums;
} accumed_t;

// Register accumulator to current finish scope
// This can be done statically in start_finish(accum_t *)
// Warning: this overwrites currently registered accum
void accum_register(accum_t ** accs, int n);

accum_t * accum_create_int(accum_op_t op, accum_mode_t mode, int init);
int accum_get_int(accum_t * acc);
void accum_put_int(accum_t * acc, int v);

accum_t * accum_create_double(accum_op_t op, accum_mode_t mode, double init);
double accum_get_double(accum_t * acc);
void accum_put_double(accum_t * acc, double v);

void accum_destroy(accum_t * acc);

#endif /* ACCUMULATOR_H_ */
