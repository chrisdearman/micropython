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
#ifndef __MICROPY_INCLUDED_PY_ASMMIPS32_H__
#define __MICROPY_INCLUDED_PY_ASMMIPS32_H__

#include "py/misc.h"

#define ASM_MIPS32_PASS_COMPUTE (1)
#define ASM_MIPS32_PASS_EMIT    (2)

// MIPS32 hardware register names
#define ASM_MIPS32_R0   (0)
#define ASM_MIPS32_R1   (1)
#define ASM_MIPS32_R2   (2)
#define ASM_MIPS32_R3   (3)
#define ASM_MIPS32_R4   (4)
#define ASM_MIPS32_R5   (5)
#define ASM_MIPS32_R6   (6)
#define ASM_MIPS32_R7   (7)
#define ASM_MIPS32_R8   (8)
#define ASM_MIPS32_R9   (9)
#define ASM_MIPS32_R10 (10)
#define ASM_MIPS32_R11 (11)
#define ASM_MIPS32_R12 (12)
#define ASM_MIPS32_R13 (13)
#define ASM_MIPS32_R14 (14)
#define ASM_MIPS32_R15 (15)
#define ASM_MIPS32_R16 (16)
#define ASM_MIPS32_R17 (17)
#define ASM_MIPS32_R18 (18)
#define ASM_MIPS32_R19 (19)
#define ASM_MIPS32_R20 (20)
#define ASM_MIPS32_R21 (21)
#define ASM_MIPS32_R22 (22)
#define ASM_MIPS32_R23 (23)
#define ASM_MIPS32_R24 (24)
#define ASM_MIPS32_R25 (25)
#define ASM_MIPS32_R26 (26)
#define ASM_MIPS32_R27 (27)
#define ASM_MIPS32_R28 (28)
#define ASM_MIPS32_R29 (29)
#define ASM_MIPS32_R30 (30)
#define ASM_MIPS32_R31 (31)

// MIPS32 software register names (O32)
#define ASM_MIPS32_REG_ZERO ASM_MIPS32_R0
#define ASM_MIPS32_REG_AT   ASM_MIPS32_R1
#define ASM_MIPS32_REG_V0   ASM_MIPS32_R2
#define ASM_MIPS32_REG_V1   ASM_MIPS32_R3
#define ASM_MIPS32_REG_A0   ASM_MIPS32_R4
#define ASM_MIPS32_REG_A1   ASM_MIPS32_R5
#define ASM_MIPS32_REG_A2   ASM_MIPS32_R6
#define ASM_MIPS32_REG_A3   ASM_MIPS32_R7
#define ASM_MIPS32_REG_T0   ASM_MIPS32_R8
#define ASM_MIPS32_REG_T1   ASM_MIPS32_R9
#define ASM_MIPS32_REG_T2   ASM_MIPS32_R10
#define ASM_MIPS32_REG_T3   ASM_MIPS32_R11
#define ASM_MIPS32_REG_T4   ASM_MIPS32_R12
#define ASM_MIPS32_REG_T5   ASM_MIPS32_R13
#define ASM_MIPS32_REG_T6   ASM_MIPS32_R14
#define ASM_MIPS32_REG_T7   ASM_MIPS32_R15
#define ASM_MIPS32_REG_S0   ASM_MIPS32_R16
#define ASM_MIPS32_REG_S1   ASM_MIPS32_R17
#define ASM_MIPS32_REG_S2   ASM_MIPS32_R18
#define ASM_MIPS32_REG_S3   ASM_MIPS32_R19
#define ASM_MIPS32_REG_S4   ASM_MIPS32_R20
#define ASM_MIPS32_REG_S5   ASM_MIPS32_R21
#define ASM_MIPS32_REG_S6   ASM_MIPS32_R22
#define ASM_MIPS32_REG_S7   ASM_MIPS32_R23
#define ASM_MIPS32_REG_T8   ASM_MIPS32_R24
#define ASM_MIPS32_REG_T9   ASM_MIPS32_R25
#define ASM_MIPS32_REG_GP   ASM_MIPS32_R28
#define ASM_MIPS32_REG_SP   ASM_MIPS32_R29
#define ASM_MIPS32_REG_S8   ASM_MIPS32_R30
#define ASM_MIPS32_REG_RA   ASM_MIPS32_R31

// If a frame pointer is used it will be in s8
#define ASM_MIPS32_REG_FP ASM_MIPS32_REG_S8

#define ASM_MIPS_SLT 0
#define ASM_MIPS_SGT 1
#define ASM_MIPS_SEQ 2
#define ASM_MIPS_SLE 3
#define ASM_MIPS_SGE 4
#define ASM_MIPS_SNE 5

typedef struct _asm_mips32_t asm_mips32_t;

asm_mips32_t *asm_mips32_new(mp_uint_t max_num_labels);
void asm_mips32_free(asm_mips32_t *as, bool free_code);
void asm_mips32_start_pass(asm_mips32_t *as, mp_uint_t pass);
void asm_mips32_end_pass(asm_mips32_t *as);
uint asm_mips32_get_code_size(asm_mips32_t *as);
void *asm_mips32_get_code(asm_mips32_t *as);

void asm_mips32_entry(asm_mips32_t *as, mp_int_t num_locals);
void asm_mips32_exit(asm_mips32_t *as);
void asm_mips32_label_assign(asm_mips32_t *as, mp_uint_t label);

void asm_mips32_align(asm_mips32_t* as, mp_uint_t align);
void asm_mips32_data(asm_mips32_t* as, mp_uint_t bytesize, mp_uint_t val);

void asm_mips32_break(asm_mips32_t *as);

// move
void asm_mips32_move(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs);
void asm_mips32_li(asm_mips32_t *as, mp_uint_t rd, mp_int_t imm);
void asm_mips32_mov_local_to_reg(asm_mips32_t *as, mp_int_t local_num, mp_uint_t reg);
void asm_mips32_mov_reg_to_local(asm_mips32_t *as, mp_uint_t reg, mp_int_t local_num);
void asm_mips32_mov_local_addr_to_reg(asm_mips32_t *as, mp_int_t local_num, mp_uint_t reg);

// compare
void asm_mips32_compare(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rx, mp_uint_t ry, mp_uint_t cond);

// arithmetic
void asm_mips32_addu(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt);
void asm_mips32_subu(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt);
void asm_mips32_and(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt);
void asm_mips32_xor(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt);
void asm_mips32_or(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt);
void asm_mips32_sll(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_int_t shift);
void asm_mips32_sllv(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rt, mp_uint_t rs);
void asm_mips32_sra(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_int_t shift);
void asm_mips32_srav(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rt, mp_uint_t rs);

// memory
void asm_mips32_lw(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);
void asm_mips32_lh(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);
void asm_mips32_lb(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);
void asm_mips32_sw(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);
void asm_mips32_sh(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);
void asm_mips32_sb(asm_mips32_t *as, mp_uint_t rt, mp_int_t offs, mp_uint_t base);

// control flow
void asm_mips32_bnez_label(asm_mips32_t *as, mp_uint_t reg, mp_uint_t label);
void asm_mips32_beqz_label(asm_mips32_t *as, mp_uint_t reg, mp_uint_t label);
void asm_mips32_beq_label(asm_mips32_t *as, mp_uint_t reg1, mp_uint_t reg2, mp_uint_t label);
void asm_mips32_b_label(asm_mips32_t *as, mp_uint_t label);
void asm_mips32_call_ind(asm_mips32_t *as, void *fun_ptr, mp_uint_t fun_id, mp_uint_t reg_temp, mp_uint_t reg_fntab);

#endif // __MICROPY_INCLUDED_PY_ASMMIPS32_H__
