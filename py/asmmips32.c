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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "py/mpconfig.h"

// wrapper around everything in this file
#if MICROPY_EMIT_MIPS32

#include "py/asmmips32.h"

#if 0 // print debugging info
#define DEBUG_PRINT (1)
#define DEBUG_printf DEBUG_printf
#else // don't print debugging info
#define DEBUG_PRINT (0)
#define DEBUG_printf(...) (void)0
#endif

#define SIGNED_FIT16(x) (((x) & 0xffff0000) == 0) || (((x) & 0xffff0000) == 0xffff0000)
#define UNSIGNED_FIT16(x) (((x) & 0xffff0000) == 0)

struct _asm_mips32_t {
    mp_uint_t pass;
    mp_uint_t code_offset;
    mp_uint_t code_size;
    byte *code_base;
    byte dummy_data[4];

    mp_uint_t max_num_labels;
    mp_uint_t *label_offsets;

    mp_uint_t stack_adjust;     /* frame size in words */
    mp_uint_t regsave_offset;   /* offset in words to regsave from sp */
    mp_uint_t locals_offset;    /* offset in words to locals from sp */
    mp_uint_t regsave_mask;
    mp_uint_t regsave_count;
    mp_int_t  num_locals;
};

asm_mips32_t *asm_mips32_new(mp_uint_t max_num_labels) {
    asm_mips32_t *as;

    as = m_new0(asm_mips32_t, 1);
    as->max_num_labels = max_num_labels;
    as->label_offsets = m_new(mp_uint_t, max_num_labels);

    return as;
}

void asm_mips32_free(asm_mips32_t *as, bool free_code) {
    if (free_code) {
        MP_PLAT_FREE_EXEC(as->code_base, as->code_size);
    }
    m_del(mp_uint_t, as->label_offsets, as->max_num_labels);
    m_del_obj(asm_mips32_t, as);
}

void asm_mips32_start_pass(asm_mips32_t *as, mp_uint_t pass) {
    if (pass == ASM_MIPS32_PASS_COMPUTE) {
        // reset all labels
        memset(as->label_offsets, -1, as->max_num_labels * sizeof(mp_uint_t));
    } else if (pass == ASM_MIPS32_PASS_EMIT) {
        MP_PLAT_ALLOC_EXEC(as->code_offset, (void**)&as->code_base, &as->code_size);
        if (as->code_base == NULL) {
            assert(0);
        }
        DEBUG_printf("code_size: "UINT_FMT"\n", as->code_size);
    }
    as->pass = pass;
    as->code_offset = 0;
}

#if DEBUG_PRINT
// micro disassembler for dumping generated code
static const char * const special_name[] = {
    "sll", "err", "srl", "sra", "sllv", "err", "srlv", "srav",  //7
    "jr", "jalr", "movz", "movn", "err", "err", "err", "sync",  //15
    "mfhi", "mthi", "mflo", "mtlo", "err", "err", "err", "err", //23
    "mult", "multu", "div", "divu", "madd", "maddu", "err", "err",      //31
    "add", "addu", "sub", "subu", "and", "or", "xor", "nor",    //39
    "add", "add", "slt", "sltu", "err", "err", "err", "err",    //47
};

static const char * const oprtname[64] = {
    "err", "err", "j", "jal", "beq", "bne", "blez", "bgtz",     //7
    "addi", "addiu", "slti", "sltiu", "andi", "ori", "xori", "lui",     //15
    "err", "err", "err", "err", "err", "err", "err", "err",     //23
    "llo", "lhi", "ldl", "ldr", "err", "err", "err", "err",     //31
    "lb", "lh", "lwl", "lw", "lbu", "lhu", "lwr", "err",        //39
    "sb", "sh", "swl", "sw", "err", "err", "swr", "err",        //47
    "ll", "lwc1", "err", "pref", "err", "ldc1", "err", "err",   //55
    "sc", "swc1", "err", "err", "err", "sdc1",
};

static const char * const regname[] = {
    "$0", "at", "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3",
    "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3",
    "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1",
    "gp", "sp", "fp", "ra",
};

#define RS      (regname[rs])
#define RT      (regname[rt])
#define RD      (regname[rd])

void disasm(mp_uint_t *ip, mp_uint_t insn) {
    char buf[32];
    mp_int_t op = (insn >> 26) & 0x3f;
    mp_int_t rs = (insn >> 21) & 0x1f;
    mp_int_t rt = (insn >> 16) & 0x1f;
    mp_int_t rd = (insn >> 11) & 0x1f;
    mp_int_t shf = (insn >> 6) & 0x1f;
    mp_int_t imm = (short)(insn & 0xffff);
    mp_uint_t uimm = insn & 0xffff;
    mp_uint_t func = insn & 0x3f;

    strcpy(buf, "???");
    switch (op) {
    case 0:
        // special cases first
        if (insn == 0 || insn == 0x40) {
            strcpy(buf, insn ? "ssnop" : "nop");
            break;
        }
        if (func == 37 && (rs == 0 || rt == 0)) {
            sprintf(buf, "%s %s,%s", "move", RD, regname[rs | rt]);
            break;
        }
        switch (func) {
        case 0 ... 3:
            sprintf(buf, "%s %s,%s,"INT_FMT, special_name[func], RD, RT, shf);
            break;
        case 4 ... 7:
            sprintf(buf, "%s %s,%s,%s", special_name[func], RD, RT, RS);
            break;
        case 8:
            sprintf(buf, "%s %s", special_name[func], RS);
            break;
        case 9:
            if (rd == 31) {
                sprintf(buf, "%s %s", special_name[func], RS);
            } else {
                sprintf(buf, "%s %s,%s", special_name[func], RD, RS);
            }
            break;
        case 10 ... 11:
            sprintf(buf, "%s %s,%s,%s", special_name[func], RD, RS, RT);
            break;
        case 16:
        case 18:
            sprintf(buf, "%s %s", special_name[func], RD);
            break;
        case 17:
        case 19:
            sprintf(buf, "%s %s", special_name[func], RS);
            break;
        case 24 ... 27:
            sprintf(buf, "%s %s,%s", special_name[func], RS, RT);
            break;
        case 32 ... 39:
        case 42 ... 43:
            sprintf(buf, "%s %s,%s,%s", special_name[func], RD, RS, RT);
            break;
        default:
            break;
        }
        break;
    case 4 ... 5:
        if (op == 4 && rs == 0 && rt == 0) {
            sprintf(buf, "%s %p", "b", ip + 1 + imm);
        } else if (rt == 0) {
            sprintf(buf, "%sz %s,%p", oprtname[op], RS, ip + 1 + imm);
        } else {
            sprintf(buf, "%s %s,%s,%p", oprtname[op], RS, RT, ip + 1 + imm);
        }
        break;
    case 8 ... 11:
        if (op == 9 && rs == 0) {
            sprintf(buf, "%s %s,"INT_FMT, "li", RT, imm);
            break;
        }
        sprintf(buf, "%s %s,%s,"INT_FMT, oprtname[op], RT, RS, imm);
        break;
    case 12 ... 14:
        if (op == 13 && rs == 0) {
            sprintf(buf, "%s %s,"INT_FMT, "li", RT, imm);
            break;
        }
        sprintf(buf, "%s %s,%s,"UINT_FMT, oprtname[op], RT, RS, uimm);
        break;
    case 15:
        sprintf(buf, "%s %s,"UINT_FMT, oprtname[op], RT, uimm);
        break;
    case 32 ... 46:
    case 48:                   /* ll */
    case 56:                   /* sc */
        sprintf(buf, "%s %s,"INT_FMT"(%s)", oprtname[op], RT, imm, RS);
        break;
    }
    DEBUG_printf("%p: %08x %s\n", ip, (unsigned int)insn, buf);
}
#endif

void asm_mips32_end_pass(asm_mips32_t *as) {
    if (as->pass == ASM_MIPS32_PASS_EMIT) {
#if DEBUG_PRINT
        mp_uint_t *ip = (mp_uint_t *)as->code_base;
        mp_uint_t *ilast = (mp_uint_t *)(as->code_base + as->code_size);
        DEBUG_printf("asm_mips32_end_pass as->code_base:%08x as->code_size:%08x\n", (uint)as->code_base, (uint)as->code_size);
        for (; ip < ilast; ip++) {
            disasm(ip, *ip);
        }
#endif
        // flush D-cache and invalidate I-cache
        __builtin___clear_cache (as->code_base, as->code_base + as->code_size);
    }
}

// all functions must go through this one to emit bytes
// if as->pass < ASM_MIPS32_PASS_EMIT, then this function only returns a buffer of 4 bytes length
STATIC byte *asm_mips32_get_cur_to_write_bytes(asm_mips32_t *as, size_t num_bytes_to_write) {
    //DEBUG_printf("emit %d\n", num_bytes_to_write);
    if (as->pass < ASM_MIPS32_PASS_EMIT) {
        as->code_offset += num_bytes_to_write;
        return as->dummy_data;
    } else {
        assert(as->code_offset + num_bytes_to_write <= as->code_size);
        byte *c = as->code_base + as->code_offset;
        as->code_offset += num_bytes_to_write;
        return c;
    }
}

uint asm_mips32_get_code_size(asm_mips32_t *as) {
    return as->code_size;
}

void *asm_mips32_get_code(asm_mips32_t *as) {
    return as->code_base;
}

// Insert word into instruction flow
STATIC void emit(asm_mips32_t *as, mp_uint_t op) {
    *(uint*)asm_mips32_get_cur_to_write_bytes(as, 4) = op;
}

// MIPS32 instructions
void asm_mips32_addiu(asm_mips32_t *as, mp_uint_t rt, mp_uint_t rs, mp_int_t imm) {
    assert(SIGNED_FIT16(imm));
    emit(as, 0x24000000 | (rs << 21) | (rt << 16) | (imm & 0xffff));
}

void asm_mips32_addu(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x00000021 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_and(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x00000024 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_beq(asm_mips32_t *as, mp_uint_t rs, mp_uint_t rt, mp_int_t offset) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0x10000000 | (rs << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_bne(asm_mips32_t *as, mp_uint_t rs, mp_uint_t rt, mp_int_t offset) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0x14000000 | (rs << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_jalr(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs) {
    emit(as, 0x00000009 | (rs << 21) | (rd << 11));
}

void asm_mips32_jr(asm_mips32_t *as, mp_uint_t rs) {
    emit(as, 0x00000008 | (rs << 21));
}

void asm_mips32_lb(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0x80000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_lh(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0x84000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_lui(asm_mips32_t *as, mp_uint_t rt, mp_uint_t imm) {
    assert(UNSIGNED_FIT16(imm));
    emit(as, 0x3c000000 | (rt << 16) | imm);
}

void asm_mips32_lw(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0x8c000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_nop(asm_mips32_t *as) {
    emit(as, 0x00000000);
}

void asm_mips32_or(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x00000025 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_ori(asm_mips32_t *as, mp_uint_t rt, mp_uint_t rs, mp_uint_t imm) {
    assert(UNSIGNED_FIT16(imm));
    emit(as, 0x34000000 | (rs << 21) | (rt << 16) | imm);
}

void asm_mips32_sb(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0xa0000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_sh(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0xa4000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_sll(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rt, mp_int_t sa) {
    assert(sa <= 31);
    emit(as, 0x00000000 | (rt << 16) | (rd << 11) | (sa << 6));
}

void asm_mips32_sllv(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rt, mp_uint_t rs) {
    emit(as, 0x00000004 | (rs << 21) | (rt << 16) | (rd << 11) | (rd << 11));
}

void asm_mips32_slt(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x0000002a | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_sltiu(asm_mips32_t *as, mp_uint_t rs, mp_uint_t rt, mp_uint_t imm) {
    assert(UNSIGNED_FIT16(imm));
    emit(as, 0x2c000000 |  (rs << 21) | (rt << 16) | imm);
}

void asm_mips32_srav(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rt, mp_uint_t rs) {
    emit(as, 0x00000007 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_subu(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x00000023 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_sw(asm_mips32_t *as, mp_uint_t rt, mp_int_t offset, mp_uint_t base) {
    assert(SIGNED_FIT16(offset));
    emit(as, 0xac000000 | (base << 21) | (rt << 16) | (offset & 0xffff));
}

void asm_mips32_xor(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    emit(as, 0x00000026 | (rs << 21) | (rt << 16) | (rd << 11));
}

void asm_mips32_xori(asm_mips32_t *as, mp_uint_t rs, mp_uint_t rt, mp_uint_t imm) {
    assert(UNSIGNED_FIT16(imm));
    emit(as, 0x38000000 | (rs << 21) | (rt << 16) | imm);
}

// MIPS32 macro instructions
void asm_mips32_b(asm_mips32_t *as, int offset) {
    asm_mips32_beq(as, ASM_MIPS32_REG_ZERO, ASM_MIPS32_REG_ZERO, offset);
}

void asm_mips32_beqz(asm_mips32_t *as, mp_uint_t rs, int offset) {
    asm_mips32_beq(as, rs, ASM_MIPS32_REG_ZERO, offset);
}

void asm_mips32_bnez(asm_mips32_t *as, mp_uint_t rs, int offset) {
    asm_mips32_bne(as, rs, ASM_MIPS32_REG_ZERO, offset);
}

void asm_mips32_jal(asm_mips32_t *as, mp_uint_t rs) {
    asm_mips32_jalr(as, ASM_MIPS32_REG_RA, rs);
}

void asm_mips32_li(asm_mips32_t *as, mp_uint_t rd, mp_int_t imm) {
    if (SIGNED_FIT16(imm)) {
        asm_mips32_addiu(as, rd, ASM_MIPS32_REG_ZERO, imm);
    }
    else if (UNSIGNED_FIT16(imm)) {
        asm_mips32_ori(as, rd, ASM_MIPS32_REG_ZERO, imm);
    }
    else {
        asm_mips32_lui(as, rd, (imm >> 16) & 0xffff);
        if (imm & 0xffff)
            asm_mips32_ori(as, rd, rd, imm & 0xffff);
    }
}

void asm_mips32_move(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs) {
    asm_mips32_or(as, rd, rs, ASM_MIPS32_REG_ZERO);
}

void asm_mips32_sgt(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    asm_mips32_slt(as, rd, rt, rs);
}

void asm_mips32_seq(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    asm_mips32_xor(as, rd, rs, rt);
    asm_mips32_sltiu(as, rd, rd, 1);
}

void asm_mips32_sle(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    asm_mips32_slt(as, rd, rt, rs);
    asm_mips32_xori(as, rd, rd, 1);
}

void asm_mips32_sge(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    asm_mips32_slt(as, rd, rs, rt);
    asm_mips32_xori(as, rd, rd, 1);
}

void asm_mips32_sne(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rs, mp_uint_t rt) {
    asm_mips32_xor(as, rd, rs, rt);
}

// high level helpers

// Stackframe
//           +----------+  as->stack_adjust
//           | regsaven |
//           | ...      |
//           | regsave0 |
//           +----------+  as->regsave_offset
//           |  localn  |
//           |  ...     |
//           |  local1  |
//           |  local0  |
// sp+0x10-> +---------=+  as->locals_offset (always 0x10)
//           | argsave3 |
//           | argsave2 |
//           | argsave1 |
//           | argsave0 |
//      sp-> +----------+
//
// NB
//   argsavearea is reserved for called functions

void asm_mips32_entry(asm_mips32_t *as, mp_int_t num_locals) {

    if (num_locals < 0) {
        num_locals = 0;
    }
    as->num_locals = num_locals;

    // Decide which registers need to be saved

    as->regsave_mask = 0;
    as->regsave_count = 0;

    // save ra (return address)
    // FIXME: leaf functions do not require this
    as->regsave_mask |= 1 << ASM_MIPS32_REG_RA; as->regsave_count += 1;

    // save s7 which is used to hold pointer to mp_fun_table
    // FIXME: may not be referenced
    as->regsave_mask |= 1 << ASM_MIPS32_REG_S7; as->regsave_count += 1;

    // save regs used for locals
    switch (num_locals) {
    default:
    case 3:
        as->regsave_mask |= (1 << ASM_MIPS32_REG_S2); as->regsave_count++;
        // Fall through
    case 2:
        as->regsave_mask |= (1 << ASM_MIPS32_REG_S1); as->regsave_count++;
        // Fall through
    case 1:
        as->regsave_mask |= (1 << ASM_MIPS32_REG_S0); as->regsave_count++;
        // Fall through
    case 0:
        break;
    }

    // generate a conservative O32 stackframe

    // non-leaf functions must allocate space for function call arguments
    // we don't know if this is a leaf function, but we do know
    // that the native emitter does call functions with more than
    // 3 arguments so we can just allocate the minimum of 4 words
    as->stack_adjust = 4;

    // space for locals
    as->locals_offset = as->stack_adjust;
    as->stack_adjust += num_locals;

    // space for caller saved registers
    as->regsave_offset = as->stack_adjust;
    as->stack_adjust += as->regsave_count;

    // Keep stack aligned stack to 8 bytes
    if (as->stack_adjust & 1) {
        as->stack_adjust += 1;
        as->regsave_offset += 1; // ensure that ra is saved at the top of the stack frame
    }

    if (as->pass == ASM_MIPS32_PASS_EMIT) {
        DEBUG_printf("%s stack_adjust: "UINT_FMT" locals_offset:"UINT_FMT" num_locals:"INT_FMT" regsave_offset:"UINT_FMT"\n",
                     __func__, as->stack_adjust, as->locals_offset, num_locals, as->regsave_offset);
    }

    if (as->stack_adjust) {
        asm_mips32_addiu(as, ASM_MIPS32_REG_SP, ASM_MIPS32_REG_SP, -(4 * as->stack_adjust));
    }

    for (mp_int_t i = 31, regoffset = as->regsave_offset + as->regsave_count - 1; i >= 0; i--) {
        if (as->regsave_mask & (1 << i)) {
            asm_mips32_sw(as, i, 4 * regoffset, ASM_MIPS32_REG_SP);
            regoffset--;
        }
    }
}

void asm_mips32_exit(asm_mips32_t *as) {
    for (mp_int_t i = 31, regoffset = as->regsave_offset + as->regsave_count - 1; i >= 0; i--) {
        if (as->regsave_mask & (1 << i)) {
            asm_mips32_lw(as, i, 4 * regoffset, ASM_MIPS32_REG_SP);
            regoffset--;
        }
    }

    asm_mips32_jr(as, ASM_MIPS32_REG_RA);
    if (as->stack_adjust) {
        asm_mips32_addiu(as, ASM_MIPS32_REG_SP, ASM_MIPS32_REG_SP, 4 * as->stack_adjust);
    }
    else {
        asm_mips32_nop(as);
    }
}

void asm_mips32_label_assign(asm_mips32_t *as, mp_uint_t label) {
    assert(label < as->max_num_labels);
    if (as->pass < ASM_MIPS32_PASS_EMIT) {
        // assign label offset
        assert(as->label_offsets[label] == -1);
        as->label_offsets[label] = as->code_offset;
    } else {
        // ensure label offset has not changed from PASS_COMPUTE to PASS_EMIT
        if (as->label_offsets[label] != as->code_offset) {
            DEBUG_printf("l"UINT_FMT": (at %ld=%ld)\n", label, as->label_offsets[label], as->code_offset);
        }
        assert(as->label_offsets[label] == as->code_offset);
    }
}

void asm_mips32_align(asm_mips32_t* as, mp_uint_t align) {
    // TODO fill unused data with NOPs?
    as->code_offset = (as->code_offset + align - 1) & (~(align - 1));
}

// FIXME: only handles 16k functions
void asm_mips32_call_ind(asm_mips32_t *as, void *fun_ptr, mp_uint_t fun_id, mp_uint_t reg_fntab, mp_uint_t reg_temp) {
    assert(fun_id < 0x2000);
    asm_mips32_lw(as, reg_temp, 4 * fun_id, reg_fntab);
    asm_mips32_jal(as, reg_temp);
    asm_mips32_nop(as);
}

// FIXME: only handles branches that are within +- 128kB
STATIC mp_int_t branch_offset(asm_mips32_t *as, mp_uint_t label) {
    assert(label < as->max_num_labels);
    mp_uint_t dest = as->label_offsets[label];
    mp_int_t rel = dest - as->code_offset;

    rel -= 4;  // branch target is relative to pc+4
    rel >>= 2; // bottom two bits of offset are implicitly zero

    if (as->pass == ASM_MIPS32_PASS_EMIT) {
        assert(dest != -1);
    }

    // For forward branches just generate a branch to self initially
    if (dest == -1) {
        rel = -1;
    }
    assert(SIGNED_FIT16(rel));
    return rel;
}

void asm_mips32_b_label(asm_mips32_t *as, mp_uint_t label) {
    mp_int_t rel = branch_offset(as, label);
    asm_mips32_b(as, rel);
    asm_mips32_nop(as);
}

void asm_mips32_beq_label(asm_mips32_t *as, mp_uint_t r1, mp_uint_t r2, mp_uint_t label) {
    mp_int_t rel = branch_offset(as, label);
    asm_mips32_beq(as, r1, r2, rel);
    asm_mips32_nop(as);
}

void asm_mips32_beqz_label(asm_mips32_t *as, mp_uint_t reg, mp_uint_t label) {
    mp_int_t rel = branch_offset(as, label);
    asm_mips32_beqz(as, reg, rel);
    asm_mips32_nop(as);
}

void asm_mips32_bnez_label(asm_mips32_t *as, mp_uint_t reg, mp_uint_t label) {
    mp_int_t rel = branch_offset(as, label);
    asm_mips32_bnez(as, reg, rel);
    asm_mips32_nop(as);
}

void asm_mips32_compare(asm_mips32_t *as, mp_uint_t rd, mp_uint_t rx, mp_uint_t ry, mp_uint_t cond) {
    switch (cond) {
    case ASM_MIPS_SLT:
        asm_mips32_slt(as, rd, rx, ry);
        break;
    case ASM_MIPS_SGT:
        asm_mips32_sgt(as, rd, rx, ry);
        break;
    case ASM_MIPS_SEQ:
        asm_mips32_seq(as, rd, rx, ry);
        break;
    case ASM_MIPS_SLE:
        asm_mips32_sle(as, rd, rx, ry);
        break;
    case ASM_MIPS_SGE:
        asm_mips32_sge(as, rd, rx, ry);
        break;
    case ASM_MIPS_SNE:
        asm_mips32_sne(as, rd, rx, ry);
        break;
    default:
        assert(0);
    }
}

static mp_int_t local_num_to_sp_offset(asm_mips32_t *as, mp_int_t local_num) {
    mp_int_t offset;
    if (as->pass == ASM_MIPS32_PASS_EMIT) {
        assert(0 <= local_num && local_num < as->num_locals);
    }
    offset = 4 * (as->locals_offset + local_num);
    assert(SIGNED_FIT16(offset));
    return offset;
}

// FIXME: locals are assumed to be < ~0x2000 so they can be accessed in a single instruction
void asm_mips32_mov_local_to_reg(asm_mips32_t *as, mp_int_t local_num, mp_uint_t reg) {
    asm_mips32_lw(as, reg, local_num_to_sp_offset(as, local_num), ASM_MIPS32_REG_SP);
}

void asm_mips32_mov_reg_to_local(asm_mips32_t *as, mp_uint_t reg, mp_int_t local_num) {
    // FIXME assumes local_num < 0x2000
    asm_mips32_sw(as, reg, local_num_to_sp_offset(as, local_num), ASM_MIPS32_REG_SP);
}

void asm_mips32_mov_local_addr_to_reg(asm_mips32_t *as, mp_int_t local_num, mp_uint_t reg) {
    // FIXME assumes local_num < 0x2000
    asm_mips32_addiu(as, reg, ASM_MIPS32_REG_SP, local_num_to_sp_offset(as, local_num));
}

#endif // MICROPY_EMIT_MIPS32
