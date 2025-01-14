/* **********************************************************
 * Copyright (c) 2010-2011 Google, Inc.  All rights reserved.
 * Copyright (c) 2002-2010 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef _DR_IR_OPCODES_AARCH64_H_
#define _DR_IR_OPCODES_AARCH64_H_ 1


/****************************************************************************
 * OPCODES
 */
/**
 * @file dr_ir_opcodes_aarch64.h
 * @brief Instruction opcode constants for AArch64.
 */
/** Opcode constants for use in the instr_t data structure. */
enum {
/*   0 */     OP_INVALID, /**< INVALID opcode */
/*   1 */     OP_UNDECODED, /**< UNDECODED opcode */
/*   2 */     OP_CONTD, /**< CONTD opcode */
/*   3 */     OP_LABEL, /**< LABEL opcode */

/*     */     OP_add,
/*     */     OP_adr,
/*     */     OP_adrp,
/*     */     OP_b,
/*     */     OP_bcond,
/*     */     OP_bl,
/*     */     OP_blr,
/*     */     OP_br,
/*     */     OP_brk,
/*     */     OP_cbnz,
/*     */     OP_cbz,
/*     */     OP_ldr,
/*     */     OP_mov,
/*     */     OP_mrs,
/*     */     OP_msr,
/*     */     OP_nop,
/*     */     OP_ret,
/*     */     OP_str,
/*     */     OP_strh,
/*     */     OP_sub,
/*     */     OP_svc,
/*     */     OP_tbnz,
/*     */     OP_tbz,
/*     */     OP_xx,

    OP_AFTER_LAST,
    OP_FIRST = OP_LABEL + 1,      /**< First real opcode. */
    OP_LAST  = OP_AFTER_LAST - 1, /**< Last real opcode. */
};

/* alternative names */
#define OP_jmp       OP_b      /**< Platform-independent opcode name for jump. */
#define OP_jmp_short OP_b      /**< Platform-independent opcode name for short jump. */
#define OP_load      OP_ldr    /**< Platform-independent opcode name for load. */
#define OP_store     OP_str    /**< Platform-independent opcode name for store. */

/****************************************************************************/



#endif /* _DR_IR_OPCODES_AARCH64_H_ */
