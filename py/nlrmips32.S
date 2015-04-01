/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#if defined(__mips__)

// struct nlr_buf {
//    struct nlr_buf *prev;
//    unsigned int retval; // holds return value from nlr_jump(retval);
//    unsigned int state[];
// }

// the offset of nlr_top within mp_state_ctx_t
#define NLR_TOP_OFFSET (2 * 4)

#define NLR_BUF_PREV     0
#define NLR_BUF_RETVAL   4
#define NLR_BUF_STATE(n) 8+((n)*4)

    .text

    .align  4
    .global nlr_push
    .type   nlr_push, @function
nlr_push:
    // save regs
    sw   $ra, NLR_BUF_STATE(0)($a0)
    sw   $s0, NLR_BUF_STATE(1)($a0)
    sw   $s1, NLR_BUF_STATE(2)($a0)
    sw   $s2, NLR_BUF_STATE(3)($a0)
    sw   $s3, NLR_BUF_STATE(4)($a0)
    sw   $s4, NLR_BUF_STATE(5)($a0)
    sw   $s5, NLR_BUF_STATE(6)($a0)
    sw   $s6, NLR_BUF_STATE(7)($a0)
    sw   $s7, NLR_BUF_STATE(8)($a0)
    sw   $s8, NLR_BUF_STATE(9)($a0)
    sw   $sp, NLR_BUF_STATE(10)($a0)

    // link this nlr_buf into mp_state_ctx->vm.nlr_top
    la   $t0, mp_state_ctx
    lw   $t1, NLR_TOP_OFFSET($t0)
    sw   $t1, NLR_BUF_PREV($a0)
    sw   $a0, NLR_TOP_OFFSET($t0)

    // return 0 (success)
    move $v0, $0
    j    $ra
    .size   nlr_push, .-nlr_push

    .align  4
    .global nlr_pop
    .type   nlr_pop, @function
nlr_pop:

    // unlink current nlr_buf from mp_state_ctx->top
    la   $t0, mp_state_ctx
    lw   $t1, NLR_TOP_OFFSET($t0)
    lw   $t2, NLR_BUF_PREV($t1)
    sw   $t2, NLR_TOP_OFFSET($t0)

    j    $ra
    .size   nlr_pop, .-nlr_pop

    .align    4
    .global    nlr_jump
    .type    nlr_jump, @function
nlr_jump:
    la   $t0, mp_state_ctx
    lw   $t1, NLR_TOP_OFFSET($t0)
    bnez $t1, 1f
    j    nlr_jump_fail
1:
    // save return value ($a0) in nlr_buf
    sw   $a0, NLR_BUF_RETVAL($t1)
    // unlink current nlr_buf from mp_state_ctx->top
    lw   $t2, NLR_BUF_PREV($t1)
    sw   $t2, NLR_TOP_OFFSET($t0)

    // restore regs
    lw   $ra, NLR_BUF_STATE(0)($t1)
    lw   $s0, NLR_BUF_STATE(1)($t1)
    lw   $s1, NLR_BUF_STATE(2)($t1)
    lw   $s2, NLR_BUF_STATE(3)($t1)
    lw   $s3, NLR_BUF_STATE(4)($t1)
    lw   $s4, NLR_BUF_STATE(5)($t1)
    lw   $s5, NLR_BUF_STATE(6)($t1)
    lw   $s6, NLR_BUF_STATE(7)($t1)
    lw   $s7, NLR_BUF_STATE(8)($t1)
    lw   $s8, NLR_BUF_STATE(9)($t1)
    lw   $sp, NLR_BUF_STATE(10)($t1)

    li   $v0, 1                  // return 1 (non-local return)
    j    $ra
    .size   nlr_jump, .-nlr_jump

#endif // defined(__mips__)
