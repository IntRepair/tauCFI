/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2012 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/// @file xed-iclass-enum.h

// This file was automatically generated.
// Do not edit this file.

#if !defined(_XED_ICLASS_ENUM_H_)
# define _XED_ICLASS_ENUM_H_
#include "xed-common-hdrs.h"
typedef enum {
  XED_ICLASS_INVALID,
  XED_ICLASS_AAA,
  XED_ICLASS_AAD,
  XED_ICLASS_AAM,
  XED_ICLASS_AAS,
  XED_ICLASS_ADC,
  XED_ICLASS_ADD,
  XED_ICLASS_ADDPD,
  XED_ICLASS_ADDPS,
  XED_ICLASS_ADDSD,
  XED_ICLASS_ADDSS,
  XED_ICLASS_ADDSUBPD,
  XED_ICLASS_ADDSUBPS,
  XED_ICLASS_AESDEC,
  XED_ICLASS_AESDECLAST,
  XED_ICLASS_AESENC,
  XED_ICLASS_AESENCLAST,
  XED_ICLASS_AESIMC,
  XED_ICLASS_AESKEYGENASSIST,
  XED_ICLASS_AND,
  XED_ICLASS_ANDN,
  XED_ICLASS_ANDNPD,
  XED_ICLASS_ANDNPS,
  XED_ICLASS_ANDPD,
  XED_ICLASS_ANDPS,
  XED_ICLASS_ARPL,
  XED_ICLASS_BEXTR,
  XED_ICLASS_BLENDPD,
  XED_ICLASS_BLENDPS,
  XED_ICLASS_BLENDVPD,
  XED_ICLASS_BLENDVPS,
  XED_ICLASS_BLSI,
  XED_ICLASS_BLSMSK,
  XED_ICLASS_BLSR,
  XED_ICLASS_BOUND,
  XED_ICLASS_BSF,
  XED_ICLASS_BSR,
  XED_ICLASS_BSWAP,
  XED_ICLASS_BT,
  XED_ICLASS_BTC,
  XED_ICLASS_BTR,
  XED_ICLASS_BTS,
  XED_ICLASS_BZHI,
  XED_ICLASS_CALL_FAR,
  XED_ICLASS_CALL_NEAR,
  XED_ICLASS_CBW,
  XED_ICLASS_CDQ,
  XED_ICLASS_CDQE,
  XED_ICLASS_CLC,
  XED_ICLASS_CLD,
  XED_ICLASS_CLFLUSH,
  XED_ICLASS_CLGI,
  XED_ICLASS_CLI,
  XED_ICLASS_CLTS,
  XED_ICLASS_CMC,
  XED_ICLASS_CMOVB,
  XED_ICLASS_CMOVBE,
  XED_ICLASS_CMOVL,
  XED_ICLASS_CMOVLE,
  XED_ICLASS_CMOVNB,
  XED_ICLASS_CMOVNBE,
  XED_ICLASS_CMOVNL,
  XED_ICLASS_CMOVNLE,
  XED_ICLASS_CMOVNO,
  XED_ICLASS_CMOVNP,
  XED_ICLASS_CMOVNS,
  XED_ICLASS_CMOVNZ,
  XED_ICLASS_CMOVO,
  XED_ICLASS_CMOVP,
  XED_ICLASS_CMOVS,
  XED_ICLASS_CMOVZ,
  XED_ICLASS_CMP,
  XED_ICLASS_CMPPD,
  XED_ICLASS_CMPPS,
  XED_ICLASS_CMPSB,
  XED_ICLASS_CMPSD,
  XED_ICLASS_CMPSD_XMM,
  XED_ICLASS_CMPSQ,
  XED_ICLASS_CMPSS,
  XED_ICLASS_CMPSW,
  XED_ICLASS_CMPXCHG,
  XED_ICLASS_CMPXCHG16B,
  XED_ICLASS_CMPXCHG8B,
  XED_ICLASS_COMISD,
  XED_ICLASS_COMISS,
  XED_ICLASS_CPUID,
  XED_ICLASS_CQO,
  XED_ICLASS_CRC32,
  XED_ICLASS_CVTDQ2PD,
  XED_ICLASS_CVTDQ2PS,
  XED_ICLASS_CVTPD2DQ,
  XED_ICLASS_CVTPD2PI,
  XED_ICLASS_CVTPD2PS,
  XED_ICLASS_CVTPI2PD,
  XED_ICLASS_CVTPI2PS,
  XED_ICLASS_CVTPS2DQ,
  XED_ICLASS_CVTPS2PD,
  XED_ICLASS_CVTPS2PI,
  XED_ICLASS_CVTSD2SI,
  XED_ICLASS_CVTSD2SS,
  XED_ICLASS_CVTSI2SD,
  XED_ICLASS_CVTSI2SS,
  XED_ICLASS_CVTSS2SD,
  XED_ICLASS_CVTSS2SI,
  XED_ICLASS_CVTTPD2DQ,
  XED_ICLASS_CVTTPD2PI,
  XED_ICLASS_CVTTPS2DQ,
  XED_ICLASS_CVTTPS2PI,
  XED_ICLASS_CVTTSD2SI,
  XED_ICLASS_CVTTSS2SI,
  XED_ICLASS_CWD,
  XED_ICLASS_CWDE,
  XED_ICLASS_DAA,
  XED_ICLASS_DAS,
  XED_ICLASS_DEC,
  XED_ICLASS_DIV,
  XED_ICLASS_DIVPD,
  XED_ICLASS_DIVPS,
  XED_ICLASS_DIVSD,
  XED_ICLASS_DIVSS,
  XED_ICLASS_DPPD,
  XED_ICLASS_DPPS,
  XED_ICLASS_EMMS,
  XED_ICLASS_ENTER,
  XED_ICLASS_EXTRACTPS,
  XED_ICLASS_EXTRQ,
  XED_ICLASS_F2XM1,
  XED_ICLASS_FABS,
  XED_ICLASS_FADD,
  XED_ICLASS_FADDP,
  XED_ICLASS_FBLD,
  XED_ICLASS_FBSTP,
  XED_ICLASS_FCHS,
  XED_ICLASS_FCMOVB,
  XED_ICLASS_FCMOVBE,
  XED_ICLASS_FCMOVE,
  XED_ICLASS_FCMOVNB,
  XED_ICLASS_FCMOVNBE,
  XED_ICLASS_FCMOVNE,
  XED_ICLASS_FCMOVNU,
  XED_ICLASS_FCMOVU,
  XED_ICLASS_FCOM,
  XED_ICLASS_FCOMI,
  XED_ICLASS_FCOMIP,
  XED_ICLASS_FCOMP,
  XED_ICLASS_FCOMPP,
  XED_ICLASS_FCOS,
  XED_ICLASS_FDECSTP,
  XED_ICLASS_FDISI8087_NOP,
  XED_ICLASS_FDIV,
  XED_ICLASS_FDIVP,
  XED_ICLASS_FDIVR,
  XED_ICLASS_FDIVRP,
  XED_ICLASS_FEMMS,
  XED_ICLASS_FENI8087_NOP,
  XED_ICLASS_FFREE,
  XED_ICLASS_FFREEP,
  XED_ICLASS_FIADD,
  XED_ICLASS_FICOM,
  XED_ICLASS_FICOMP,
  XED_ICLASS_FIDIV,
  XED_ICLASS_FIDIVR,
  XED_ICLASS_FILD,
  XED_ICLASS_FIMUL,
  XED_ICLASS_FINCSTP,
  XED_ICLASS_FIST,
  XED_ICLASS_FISTP,
  XED_ICLASS_FISTTP,
  XED_ICLASS_FISUB,
  XED_ICLASS_FISUBR,
  XED_ICLASS_FLD,
  XED_ICLASS_FLD1,
  XED_ICLASS_FLDCW,
  XED_ICLASS_FLDENV,
  XED_ICLASS_FLDL2E,
  XED_ICLASS_FLDL2T,
  XED_ICLASS_FLDLG2,
  XED_ICLASS_FLDLN2,
  XED_ICLASS_FLDPI,
  XED_ICLASS_FLDZ,
  XED_ICLASS_FMUL,
  XED_ICLASS_FMULP,
  XED_ICLASS_FNCLEX,
  XED_ICLASS_FNINIT,
  XED_ICLASS_FNOP,
  XED_ICLASS_FNSAVE,
  XED_ICLASS_FNSTCW,
  XED_ICLASS_FNSTENV,
  XED_ICLASS_FNSTSW,
  XED_ICLASS_FPATAN,
  XED_ICLASS_FPREM,
  XED_ICLASS_FPREM1,
  XED_ICLASS_FPTAN,
  XED_ICLASS_FRNDINT,
  XED_ICLASS_FRSTOR,
  XED_ICLASS_FSCALE,
  XED_ICLASS_FSETPM287_NOP,
  XED_ICLASS_FSIN,
  XED_ICLASS_FSINCOS,
  XED_ICLASS_FSQRT,
  XED_ICLASS_FST,
  XED_ICLASS_FSTP,
  XED_ICLASS_FSTPNCE,
  XED_ICLASS_FSUB,
  XED_ICLASS_FSUBP,
  XED_ICLASS_FSUBR,
  XED_ICLASS_FSUBRP,
  XED_ICLASS_FTST,
  XED_ICLASS_FUCOM,
  XED_ICLASS_FUCOMI,
  XED_ICLASS_FUCOMIP,
  XED_ICLASS_FUCOMP,
  XED_ICLASS_FUCOMPP,
  XED_ICLASS_FWAIT,
  XED_ICLASS_FXAM,
  XED_ICLASS_FXCH,
  XED_ICLASS_FXRSTOR,
  XED_ICLASS_FXRSTOR64,
  XED_ICLASS_FXSAVE,
  XED_ICLASS_FXSAVE64,
  XED_ICLASS_FXTRACT,
  XED_ICLASS_FYL2X,
  XED_ICLASS_FYL2XP1,
  XED_ICLASS_GETSEC,
  XED_ICLASS_HADDPD,
  XED_ICLASS_HADDPS,
  XED_ICLASS_HLT,
  XED_ICLASS_HSUBPD,
  XED_ICLASS_HSUBPS,
  XED_ICLASS_IDIV,
  XED_ICLASS_IMUL,
  XED_ICLASS_IN,
  XED_ICLASS_INC,
  XED_ICLASS_INSB,
  XED_ICLASS_INSD,
  XED_ICLASS_INSERTPS,
  XED_ICLASS_INSERTQ,
  XED_ICLASS_INSW,
  XED_ICLASS_INT,
  XED_ICLASS_INT1,
  XED_ICLASS_INT3,
  XED_ICLASS_INTO,
  XED_ICLASS_INVD,
  XED_ICLASS_INVEPT,
  XED_ICLASS_INVLPG,
  XED_ICLASS_INVLPGA,
  XED_ICLASS_INVPCID,
  XED_ICLASS_INVVPID,
  XED_ICLASS_IRET,
  XED_ICLASS_IRETD,
  XED_ICLASS_IRETQ,
  XED_ICLASS_JB,
  XED_ICLASS_JBE,
  XED_ICLASS_JL,
  XED_ICLASS_JLE,
  XED_ICLASS_JMP,
  XED_ICLASS_JMP_FAR,
  XED_ICLASS_JNB,
  XED_ICLASS_JNBE,
  XED_ICLASS_JNL,
  XED_ICLASS_JNLE,
  XED_ICLASS_JNO,
  XED_ICLASS_JNP,
  XED_ICLASS_JNS,
  XED_ICLASS_JNZ,
  XED_ICLASS_JO,
  XED_ICLASS_JP,
  XED_ICLASS_JRCXZ,
  XED_ICLASS_JS,
  XED_ICLASS_JZ,
  XED_ICLASS_LAHF,
  XED_ICLASS_LAR,
  XED_ICLASS_LDDQU,
  XED_ICLASS_LDMXCSR,
  XED_ICLASS_LDS,
  XED_ICLASS_LEA,
  XED_ICLASS_LEAVE,
  XED_ICLASS_LES,
  XED_ICLASS_LFENCE,
  XED_ICLASS_LFS,
  XED_ICLASS_LGDT,
  XED_ICLASS_LGS,
  XED_ICLASS_LIDT,
  XED_ICLASS_LLDT,
  XED_ICLASS_LMSW,
  XED_ICLASS_LODSB,
  XED_ICLASS_LODSD,
  XED_ICLASS_LODSQ,
  XED_ICLASS_LODSW,
  XED_ICLASS_LOOP,
  XED_ICLASS_LOOPE,
  XED_ICLASS_LOOPNE,
  XED_ICLASS_LSL,
  XED_ICLASS_LSS,
  XED_ICLASS_LTR,
  XED_ICLASS_LZCNT,
  XED_ICLASS_MASKMOVDQU,
  XED_ICLASS_MASKMOVQ,
  XED_ICLASS_MAXPD,
  XED_ICLASS_MAXPS,
  XED_ICLASS_MAXSD,
  XED_ICLASS_MAXSS,
  XED_ICLASS_MFENCE,
  XED_ICLASS_MINPD,
  XED_ICLASS_MINPS,
  XED_ICLASS_MINSD,
  XED_ICLASS_MINSS,
  XED_ICLASS_MONITOR,
  XED_ICLASS_MOV,
  XED_ICLASS_MOVAPD,
  XED_ICLASS_MOVAPS,
  XED_ICLASS_MOVBE,
  XED_ICLASS_MOVD,
  XED_ICLASS_MOVDDUP,
  XED_ICLASS_MOVDQ2Q,
  XED_ICLASS_MOVDQA,
  XED_ICLASS_MOVDQU,
  XED_ICLASS_MOVHLPS,
  XED_ICLASS_MOVHPD,
  XED_ICLASS_MOVHPS,
  XED_ICLASS_MOVLHPS,
  XED_ICLASS_MOVLPD,
  XED_ICLASS_MOVLPS,
  XED_ICLASS_MOVMSKPD,
  XED_ICLASS_MOVMSKPS,
  XED_ICLASS_MOVNTDQ,
  XED_ICLASS_MOVNTDQA,
  XED_ICLASS_MOVNTI,
  XED_ICLASS_MOVNTPD,
  XED_ICLASS_MOVNTPS,
  XED_ICLASS_MOVNTQ,
  XED_ICLASS_MOVNTSD,
  XED_ICLASS_MOVNTSS,
  XED_ICLASS_MOVQ,
  XED_ICLASS_MOVQ2DQ,
  XED_ICLASS_MOVSB,
  XED_ICLASS_MOVSD,
  XED_ICLASS_MOVSD_XMM,
  XED_ICLASS_MOVSHDUP,
  XED_ICLASS_MOVSLDUP,
  XED_ICLASS_MOVSQ,
  XED_ICLASS_MOVSS,
  XED_ICLASS_MOVSW,
  XED_ICLASS_MOVSX,
  XED_ICLASS_MOVSXD,
  XED_ICLASS_MOVUPD,
  XED_ICLASS_MOVUPS,
  XED_ICLASS_MOVZX,
  XED_ICLASS_MOV_CR,
  XED_ICLASS_MOV_DR,
  XED_ICLASS_MPSADBW,
  XED_ICLASS_MUL,
  XED_ICLASS_MULPD,
  XED_ICLASS_MULPS,
  XED_ICLASS_MULSD,
  XED_ICLASS_MULSS,
  XED_ICLASS_MULX,
  XED_ICLASS_MWAIT,
  XED_ICLASS_NEG,
  XED_ICLASS_NOP,
  XED_ICLASS_NOP2,
  XED_ICLASS_NOP3,
  XED_ICLASS_NOP4,
  XED_ICLASS_NOP5,
  XED_ICLASS_NOP6,
  XED_ICLASS_NOP7,
  XED_ICLASS_NOP8,
  XED_ICLASS_NOP9,
  XED_ICLASS_NOT,
  XED_ICLASS_OR,
  XED_ICLASS_ORPD,
  XED_ICLASS_ORPS,
  XED_ICLASS_OUT,
  XED_ICLASS_OUTSB,
  XED_ICLASS_OUTSD,
  XED_ICLASS_OUTSW,
  XED_ICLASS_PABSB,
  XED_ICLASS_PABSD,
  XED_ICLASS_PABSW,
  XED_ICLASS_PACKSSDW,
  XED_ICLASS_PACKSSWB,
  XED_ICLASS_PACKUSDW,
  XED_ICLASS_PACKUSWB,
  XED_ICLASS_PADDB,
  XED_ICLASS_PADDD,
  XED_ICLASS_PADDQ,
  XED_ICLASS_PADDSB,
  XED_ICLASS_PADDSW,
  XED_ICLASS_PADDUSB,
  XED_ICLASS_PADDUSW,
  XED_ICLASS_PADDW,
  XED_ICLASS_PALIGNR,
  XED_ICLASS_PAND,
  XED_ICLASS_PANDN,
  XED_ICLASS_PAUSE,
  XED_ICLASS_PAVGB,
  XED_ICLASS_PAVGUSB,
  XED_ICLASS_PAVGW,
  XED_ICLASS_PBLENDVB,
  XED_ICLASS_PBLENDW,
  XED_ICLASS_PCLMULQDQ,
  XED_ICLASS_PCMPEQB,
  XED_ICLASS_PCMPEQD,
  XED_ICLASS_PCMPEQQ,
  XED_ICLASS_PCMPEQW,
  XED_ICLASS_PCMPESTRI,
  XED_ICLASS_PCMPESTRM,
  XED_ICLASS_PCMPGTB,
  XED_ICLASS_PCMPGTD,
  XED_ICLASS_PCMPGTQ,
  XED_ICLASS_PCMPGTW,
  XED_ICLASS_PCMPISTRI,
  XED_ICLASS_PCMPISTRM,
  XED_ICLASS_PDEP,
  XED_ICLASS_PEXT,
  XED_ICLASS_PEXTRB,
  XED_ICLASS_PEXTRD,
  XED_ICLASS_PEXTRQ,
  XED_ICLASS_PEXTRW,
  XED_ICLASS_PF2ID,
  XED_ICLASS_PF2IW,
  XED_ICLASS_PFACC,
  XED_ICLASS_PFADD,
  XED_ICLASS_PFCMPEQ,
  XED_ICLASS_PFCMPGE,
  XED_ICLASS_PFCMPGT,
  XED_ICLASS_PFCPIT1,
  XED_ICLASS_PFMAX,
  XED_ICLASS_PFMIN,
  XED_ICLASS_PFMUL,
  XED_ICLASS_PFNACC,
  XED_ICLASS_PFPNACC,
  XED_ICLASS_PFRCP,
  XED_ICLASS_PFRCPIT2,
  XED_ICLASS_PFRSQIT1,
  XED_ICLASS_PFSQRT,
  XED_ICLASS_PFSUB,
  XED_ICLASS_PFSUBR,
  XED_ICLASS_PHADDD,
  XED_ICLASS_PHADDSW,
  XED_ICLASS_PHADDW,
  XED_ICLASS_PHMINPOSUW,
  XED_ICLASS_PHSUBD,
  XED_ICLASS_PHSUBSW,
  XED_ICLASS_PHSUBW,
  XED_ICLASS_PI2FD,
  XED_ICLASS_PI2FW,
  XED_ICLASS_PINSRB,
  XED_ICLASS_PINSRD,
  XED_ICLASS_PINSRQ,
  XED_ICLASS_PINSRW,
  XED_ICLASS_PMADDUBSW,
  XED_ICLASS_PMADDWD,
  XED_ICLASS_PMAXSB,
  XED_ICLASS_PMAXSD,
  XED_ICLASS_PMAXSW,
  XED_ICLASS_PMAXUB,
  XED_ICLASS_PMAXUD,
  XED_ICLASS_PMAXUW,
  XED_ICLASS_PMINSB,
  XED_ICLASS_PMINSD,
  XED_ICLASS_PMINSW,
  XED_ICLASS_PMINUB,
  XED_ICLASS_PMINUD,
  XED_ICLASS_PMINUW,
  XED_ICLASS_PMOVMSKB,
  XED_ICLASS_PMOVSXBD,
  XED_ICLASS_PMOVSXBQ,
  XED_ICLASS_PMOVSXBW,
  XED_ICLASS_PMOVSXDQ,
  XED_ICLASS_PMOVSXWD,
  XED_ICLASS_PMOVSXWQ,
  XED_ICLASS_PMOVZXBD,
  XED_ICLASS_PMOVZXBQ,
  XED_ICLASS_PMOVZXBW,
  XED_ICLASS_PMOVZXDQ,
  XED_ICLASS_PMOVZXWD,
  XED_ICLASS_PMOVZXWQ,
  XED_ICLASS_PMULDQ,
  XED_ICLASS_PMULHRSW,
  XED_ICLASS_PMULHRW,
  XED_ICLASS_PMULHUW,
  XED_ICLASS_PMULHW,
  XED_ICLASS_PMULLD,
  XED_ICLASS_PMULLW,
  XED_ICLASS_PMULUDQ,
  XED_ICLASS_POP,
  XED_ICLASS_POPA,
  XED_ICLASS_POPAD,
  XED_ICLASS_POPCNT,
  XED_ICLASS_POPF,
  XED_ICLASS_POPFD,
  XED_ICLASS_POPFQ,
  XED_ICLASS_POR,
  XED_ICLASS_PREFETCHNTA,
  XED_ICLASS_PREFETCHT0,
  XED_ICLASS_PREFETCHT1,
  XED_ICLASS_PREFETCHT2,
  XED_ICLASS_PREFETCH_EXCLUSIVE,
  XED_ICLASS_PREFETCH_MODIFIED,
  XED_ICLASS_PREFETCH_RESERVED,
  XED_ICLASS_PSADBW,
  XED_ICLASS_PSHUFB,
  XED_ICLASS_PSHUFD,
  XED_ICLASS_PSHUFHW,
  XED_ICLASS_PSHUFLW,
  XED_ICLASS_PSHUFW,
  XED_ICLASS_PSIGNB,
  XED_ICLASS_PSIGND,
  XED_ICLASS_PSIGNW,
  XED_ICLASS_PSLLD,
  XED_ICLASS_PSLLDQ,
  XED_ICLASS_PSLLQ,
  XED_ICLASS_PSLLW,
  XED_ICLASS_PSRAD,
  XED_ICLASS_PSRAW,
  XED_ICLASS_PSRLD,
  XED_ICLASS_PSRLDQ,
  XED_ICLASS_PSRLQ,
  XED_ICLASS_PSRLW,
  XED_ICLASS_PSUBB,
  XED_ICLASS_PSUBD,
  XED_ICLASS_PSUBQ,
  XED_ICLASS_PSUBSB,
  XED_ICLASS_PSUBSW,
  XED_ICLASS_PSUBUSB,
  XED_ICLASS_PSUBUSW,
  XED_ICLASS_PSUBW,
  XED_ICLASS_PSWAPD,
  XED_ICLASS_PTEST,
  XED_ICLASS_PUNPCKHBW,
  XED_ICLASS_PUNPCKHDQ,
  XED_ICLASS_PUNPCKHQDQ,
  XED_ICLASS_PUNPCKHWD,
  XED_ICLASS_PUNPCKLBW,
  XED_ICLASS_PUNPCKLDQ,
  XED_ICLASS_PUNPCKLQDQ,
  XED_ICLASS_PUNPCKLWD,
  XED_ICLASS_PUSH,
  XED_ICLASS_PUSHA,
  XED_ICLASS_PUSHAD,
  XED_ICLASS_PUSHF,
  XED_ICLASS_PUSHFD,
  XED_ICLASS_PUSHFQ,
  XED_ICLASS_PXOR,
  XED_ICLASS_RCL,
  XED_ICLASS_RCPPS,
  XED_ICLASS_RCPSS,
  XED_ICLASS_RCR,
  XED_ICLASS_RDFSBASE,
  XED_ICLASS_RDGSBASE,
  XED_ICLASS_RDMSR,
  XED_ICLASS_RDPMC,
  XED_ICLASS_RDRAND,
  XED_ICLASS_RDTSC,
  XED_ICLASS_RDTSCP,
  XED_ICLASS_RET_FAR,
  XED_ICLASS_RET_NEAR,
  XED_ICLASS_ROL,
  XED_ICLASS_ROR,
  XED_ICLASS_RORX,
  XED_ICLASS_ROUNDPD,
  XED_ICLASS_ROUNDPS,
  XED_ICLASS_ROUNDSD,
  XED_ICLASS_ROUNDSS,
  XED_ICLASS_RSM,
  XED_ICLASS_RSQRTPS,
  XED_ICLASS_RSQRTSS,
  XED_ICLASS_SAHF,
  XED_ICLASS_SALC,
  XED_ICLASS_SAR,
  XED_ICLASS_SARX,
  XED_ICLASS_SBB,
  XED_ICLASS_SCASB,
  XED_ICLASS_SCASD,
  XED_ICLASS_SCASQ,
  XED_ICLASS_SCASW,
  XED_ICLASS_SETB,
  XED_ICLASS_SETBE,
  XED_ICLASS_SETL,
  XED_ICLASS_SETLE,
  XED_ICLASS_SETNB,
  XED_ICLASS_SETNBE,
  XED_ICLASS_SETNL,
  XED_ICLASS_SETNLE,
  XED_ICLASS_SETNO,
  XED_ICLASS_SETNP,
  XED_ICLASS_SETNS,
  XED_ICLASS_SETNZ,
  XED_ICLASS_SETO,
  XED_ICLASS_SETP,
  XED_ICLASS_SETS,
  XED_ICLASS_SETZ,
  XED_ICLASS_SFENCE,
  XED_ICLASS_SGDT,
  XED_ICLASS_SHL,
  XED_ICLASS_SHLD,
  XED_ICLASS_SHLX,
  XED_ICLASS_SHR,
  XED_ICLASS_SHRD,
  XED_ICLASS_SHRX,
  XED_ICLASS_SHUFPD,
  XED_ICLASS_SHUFPS,
  XED_ICLASS_SIDT,
  XED_ICLASS_SKINIT,
  XED_ICLASS_SLDT,
  XED_ICLASS_SMSW,
  XED_ICLASS_SQRTPD,
  XED_ICLASS_SQRTPS,
  XED_ICLASS_SQRTSD,
  XED_ICLASS_SQRTSS,
  XED_ICLASS_STC,
  XED_ICLASS_STD,
  XED_ICLASS_STGI,
  XED_ICLASS_STI,
  XED_ICLASS_STMXCSR,
  XED_ICLASS_STOSB,
  XED_ICLASS_STOSD,
  XED_ICLASS_STOSQ,
  XED_ICLASS_STOSW,
  XED_ICLASS_STR,
  XED_ICLASS_SUB,
  XED_ICLASS_SUBPD,
  XED_ICLASS_SUBPS,
  XED_ICLASS_SUBSD,
  XED_ICLASS_SUBSS,
  XED_ICLASS_SWAPGS,
  XED_ICLASS_SYSCALL,
  XED_ICLASS_SYSCALL_AMD,
  XED_ICLASS_SYSENTER,
  XED_ICLASS_SYSEXIT,
  XED_ICLASS_SYSRET,
  XED_ICLASS_SYSRET_AMD,
  XED_ICLASS_TEST,
  XED_ICLASS_TZCNT,
  XED_ICLASS_UCOMISD,
  XED_ICLASS_UCOMISS,
  XED_ICLASS_UD2,
  XED_ICLASS_UNPCKHPD,
  XED_ICLASS_UNPCKHPS,
  XED_ICLASS_UNPCKLPD,
  XED_ICLASS_UNPCKLPS,
  XED_ICLASS_VADDPD,
  XED_ICLASS_VADDPS,
  XED_ICLASS_VADDSD,
  XED_ICLASS_VADDSS,
  XED_ICLASS_VADDSUBPD,
  XED_ICLASS_VADDSUBPS,
  XED_ICLASS_VAESDEC,
  XED_ICLASS_VAESDECLAST,
  XED_ICLASS_VAESENC,
  XED_ICLASS_VAESENCLAST,
  XED_ICLASS_VAESIMC,
  XED_ICLASS_VAESKEYGENASSIST,
  XED_ICLASS_VANDNPD,
  XED_ICLASS_VANDNPS,
  XED_ICLASS_VANDPD,
  XED_ICLASS_VANDPS,
  XED_ICLASS_VBLENDPD,
  XED_ICLASS_VBLENDPS,
  XED_ICLASS_VBLENDVPD,
  XED_ICLASS_VBLENDVPS,
  XED_ICLASS_VBROADCASTF128,
  XED_ICLASS_VBROADCASTI128,
  XED_ICLASS_VBROADCASTSD,
  XED_ICLASS_VBROADCASTSS,
  XED_ICLASS_VCMPPD,
  XED_ICLASS_VCMPPS,
  XED_ICLASS_VCMPSD,
  XED_ICLASS_VCMPSS,
  XED_ICLASS_VCOMISD,
  XED_ICLASS_VCOMISS,
  XED_ICLASS_VCVTDQ2PD,
  XED_ICLASS_VCVTDQ2PS,
  XED_ICLASS_VCVTPD2DQ,
  XED_ICLASS_VCVTPD2PS,
  XED_ICLASS_VCVTPH2PS,
  XED_ICLASS_VCVTPS2DQ,
  XED_ICLASS_VCVTPS2PD,
  XED_ICLASS_VCVTPS2PH,
  XED_ICLASS_VCVTSD2SI,
  XED_ICLASS_VCVTSD2SS,
  XED_ICLASS_VCVTSI2SD,
  XED_ICLASS_VCVTSI2SS,
  XED_ICLASS_VCVTSS2SD,
  XED_ICLASS_VCVTSS2SI,
  XED_ICLASS_VCVTTPD2DQ,
  XED_ICLASS_VCVTTPS2DQ,
  XED_ICLASS_VCVTTSD2SI,
  XED_ICLASS_VCVTTSS2SI,
  XED_ICLASS_VDIVPD,
  XED_ICLASS_VDIVPS,
  XED_ICLASS_VDIVSD,
  XED_ICLASS_VDIVSS,
  XED_ICLASS_VDPPD,
  XED_ICLASS_VDPPS,
  XED_ICLASS_VERR,
  XED_ICLASS_VERW,
  XED_ICLASS_VEXTRACTF128,
  XED_ICLASS_VEXTRACTI128,
  XED_ICLASS_VEXTRACTPS,
  XED_ICLASS_VFMADD132PD,
  XED_ICLASS_VFMADD132PS,
  XED_ICLASS_VFMADD132SD,
  XED_ICLASS_VFMADD132SS,
  XED_ICLASS_VFMADD213PD,
  XED_ICLASS_VFMADD213PS,
  XED_ICLASS_VFMADD213SD,
  XED_ICLASS_VFMADD213SS,
  XED_ICLASS_VFMADD231PD,
  XED_ICLASS_VFMADD231PS,
  XED_ICLASS_VFMADD231SD,
  XED_ICLASS_VFMADD231SS,
  XED_ICLASS_VFMADDSUB132PD,
  XED_ICLASS_VFMADDSUB132PS,
  XED_ICLASS_VFMADDSUB213PD,
  XED_ICLASS_VFMADDSUB213PS,
  XED_ICLASS_VFMADDSUB231PD,
  XED_ICLASS_VFMADDSUB231PS,
  XED_ICLASS_VFMSUB132PD,
  XED_ICLASS_VFMSUB132PS,
  XED_ICLASS_VFMSUB132SD,
  XED_ICLASS_VFMSUB132SS,
  XED_ICLASS_VFMSUB213PD,
  XED_ICLASS_VFMSUB213PS,
  XED_ICLASS_VFMSUB213SD,
  XED_ICLASS_VFMSUB213SS,
  XED_ICLASS_VFMSUB231PD,
  XED_ICLASS_VFMSUB231PS,
  XED_ICLASS_VFMSUB231SD,
  XED_ICLASS_VFMSUB231SS,
  XED_ICLASS_VFMSUBADD132PD,
  XED_ICLASS_VFMSUBADD132PS,
  XED_ICLASS_VFMSUBADD213PD,
  XED_ICLASS_VFMSUBADD213PS,
  XED_ICLASS_VFMSUBADD231PD,
  XED_ICLASS_VFMSUBADD231PS,
  XED_ICLASS_VFNMADD132PD,
  XED_ICLASS_VFNMADD132PS,
  XED_ICLASS_VFNMADD132SD,
  XED_ICLASS_VFNMADD132SS,
  XED_ICLASS_VFNMADD213PD,
  XED_ICLASS_VFNMADD213PS,
  XED_ICLASS_VFNMADD213SD,
  XED_ICLASS_VFNMADD213SS,
  XED_ICLASS_VFNMADD231PD,
  XED_ICLASS_VFNMADD231PS,
  XED_ICLASS_VFNMADD231SD,
  XED_ICLASS_VFNMADD231SS,
  XED_ICLASS_VFNMSUB132PD,
  XED_ICLASS_VFNMSUB132PS,
  XED_ICLASS_VFNMSUB132SD,
  XED_ICLASS_VFNMSUB132SS,
  XED_ICLASS_VFNMSUB213PD,
  XED_ICLASS_VFNMSUB213PS,
  XED_ICLASS_VFNMSUB213SD,
  XED_ICLASS_VFNMSUB213SS,
  XED_ICLASS_VFNMSUB231PD,
  XED_ICLASS_VFNMSUB231PS,
  XED_ICLASS_VFNMSUB231SD,
  XED_ICLASS_VFNMSUB231SS,
  XED_ICLASS_VGATHERDPD,
  XED_ICLASS_VGATHERDPS,
  XED_ICLASS_VGATHERQPD,
  XED_ICLASS_VGATHERQPS,
  XED_ICLASS_VHADDPD,
  XED_ICLASS_VHADDPS,
  XED_ICLASS_VHSUBPD,
  XED_ICLASS_VHSUBPS,
  XED_ICLASS_VINSERTF128,
  XED_ICLASS_VINSERTI128,
  XED_ICLASS_VINSERTPS,
  XED_ICLASS_VLDDQU,
  XED_ICLASS_VLDMXCSR,
  XED_ICLASS_VMASKMOVDQU,
  XED_ICLASS_VMASKMOVPD,
  XED_ICLASS_VMASKMOVPS,
  XED_ICLASS_VMAXPD,
  XED_ICLASS_VMAXPS,
  XED_ICLASS_VMAXSD,
  XED_ICLASS_VMAXSS,
  XED_ICLASS_VMCALL,
  XED_ICLASS_VMCLEAR,
  XED_ICLASS_VMFUNC,
  XED_ICLASS_VMINPD,
  XED_ICLASS_VMINPS,
  XED_ICLASS_VMINSD,
  XED_ICLASS_VMINSS,
  XED_ICLASS_VMLAUNCH,
  XED_ICLASS_VMLOAD,
  XED_ICLASS_VMMCALL,
  XED_ICLASS_VMOVAPD,
  XED_ICLASS_VMOVAPS,
  XED_ICLASS_VMOVD,
  XED_ICLASS_VMOVDDUP,
  XED_ICLASS_VMOVDQA,
  XED_ICLASS_VMOVDQU,
  XED_ICLASS_VMOVHLPS,
  XED_ICLASS_VMOVHPD,
  XED_ICLASS_VMOVHPS,
  XED_ICLASS_VMOVLHPS,
  XED_ICLASS_VMOVLPD,
  XED_ICLASS_VMOVLPS,
  XED_ICLASS_VMOVMSKPD,
  XED_ICLASS_VMOVMSKPS,
  XED_ICLASS_VMOVNTDQ,
  XED_ICLASS_VMOVNTDQA,
  XED_ICLASS_VMOVNTPD,
  XED_ICLASS_VMOVNTPS,
  XED_ICLASS_VMOVQ,
  XED_ICLASS_VMOVSD,
  XED_ICLASS_VMOVSHDUP,
  XED_ICLASS_VMOVSLDUP,
  XED_ICLASS_VMOVSS,
  XED_ICLASS_VMOVUPD,
  XED_ICLASS_VMOVUPS,
  XED_ICLASS_VMPSADBW,
  XED_ICLASS_VMPTRLD,
  XED_ICLASS_VMPTRST,
  XED_ICLASS_VMREAD,
  XED_ICLASS_VMRESUME,
  XED_ICLASS_VMRUN,
  XED_ICLASS_VMSAVE,
  XED_ICLASS_VMULPD,
  XED_ICLASS_VMULPS,
  XED_ICLASS_VMULSD,
  XED_ICLASS_VMULSS,
  XED_ICLASS_VMWRITE,
  XED_ICLASS_VMXOFF,
  XED_ICLASS_VMXON,
  XED_ICLASS_VORPD,
  XED_ICLASS_VORPS,
  XED_ICLASS_VPABSB,
  XED_ICLASS_VPABSD,
  XED_ICLASS_VPABSW,
  XED_ICLASS_VPACKSSDW,
  XED_ICLASS_VPACKSSWB,
  XED_ICLASS_VPACKUSDW,
  XED_ICLASS_VPACKUSWB,
  XED_ICLASS_VPADDB,
  XED_ICLASS_VPADDD,
  XED_ICLASS_VPADDQ,
  XED_ICLASS_VPADDSB,
  XED_ICLASS_VPADDSW,
  XED_ICLASS_VPADDUSB,
  XED_ICLASS_VPADDUSW,
  XED_ICLASS_VPADDW,
  XED_ICLASS_VPALIGNR,
  XED_ICLASS_VPAND,
  XED_ICLASS_VPANDN,
  XED_ICLASS_VPAVGB,
  XED_ICLASS_VPAVGW,
  XED_ICLASS_VPBLENDD,
  XED_ICLASS_VPBLENDVB,
  XED_ICLASS_VPBLENDW,
  XED_ICLASS_VPBROADCASTB,
  XED_ICLASS_VPBROADCASTD,
  XED_ICLASS_VPBROADCASTQ,
  XED_ICLASS_VPBROADCASTW,
  XED_ICLASS_VPCLMULQDQ,
  XED_ICLASS_VPCMPEQB,
  XED_ICLASS_VPCMPEQD,
  XED_ICLASS_VPCMPEQQ,
  XED_ICLASS_VPCMPEQW,
  XED_ICLASS_VPCMPESTRI,
  XED_ICLASS_VPCMPESTRM,
  XED_ICLASS_VPCMPGTB,
  XED_ICLASS_VPCMPGTD,
  XED_ICLASS_VPCMPGTQ,
  XED_ICLASS_VPCMPGTW,
  XED_ICLASS_VPCMPISTRI,
  XED_ICLASS_VPCMPISTRM,
  XED_ICLASS_VPERM2F128,
  XED_ICLASS_VPERM2I128,
  XED_ICLASS_VPERMD,
  XED_ICLASS_VPERMILPD,
  XED_ICLASS_VPERMILPS,
  XED_ICLASS_VPERMPD,
  XED_ICLASS_VPERMPS,
  XED_ICLASS_VPERMQ,
  XED_ICLASS_VPEXTRB,
  XED_ICLASS_VPEXTRD,
  XED_ICLASS_VPEXTRQ,
  XED_ICLASS_VPEXTRW,
  XED_ICLASS_VPGATHERDD,
  XED_ICLASS_VPGATHERDQ,
  XED_ICLASS_VPGATHERQD,
  XED_ICLASS_VPGATHERQQ,
  XED_ICLASS_VPHADDD,
  XED_ICLASS_VPHADDSW,
  XED_ICLASS_VPHADDW,
  XED_ICLASS_VPHMINPOSUW,
  XED_ICLASS_VPHSUBD,
  XED_ICLASS_VPHSUBSW,
  XED_ICLASS_VPHSUBW,
  XED_ICLASS_VPINSRB,
  XED_ICLASS_VPINSRD,
  XED_ICLASS_VPINSRQ,
  XED_ICLASS_VPINSRW,
  XED_ICLASS_VPMADDUBSW,
  XED_ICLASS_VPMADDWD,
  XED_ICLASS_VPMASKMOVD,
  XED_ICLASS_VPMASKMOVQ,
  XED_ICLASS_VPMAXSB,
  XED_ICLASS_VPMAXSD,
  XED_ICLASS_VPMAXSW,
  XED_ICLASS_VPMAXUB,
  XED_ICLASS_VPMAXUD,
  XED_ICLASS_VPMAXUW,
  XED_ICLASS_VPMINSB,
  XED_ICLASS_VPMINSD,
  XED_ICLASS_VPMINSW,
  XED_ICLASS_VPMINUB,
  XED_ICLASS_VPMINUD,
  XED_ICLASS_VPMINUW,
  XED_ICLASS_VPMOVMSKB,
  XED_ICLASS_VPMOVSXBD,
  XED_ICLASS_VPMOVSXBQ,
  XED_ICLASS_VPMOVSXBW,
  XED_ICLASS_VPMOVSXDQ,
  XED_ICLASS_VPMOVSXWD,
  XED_ICLASS_VPMOVSXWQ,
  XED_ICLASS_VPMOVZXBD,
  XED_ICLASS_VPMOVZXBQ,
  XED_ICLASS_VPMOVZXBW,
  XED_ICLASS_VPMOVZXDQ,
  XED_ICLASS_VPMOVZXWD,
  XED_ICLASS_VPMOVZXWQ,
  XED_ICLASS_VPMULDQ,
  XED_ICLASS_VPMULHRSW,
  XED_ICLASS_VPMULHUW,
  XED_ICLASS_VPMULHW,
  XED_ICLASS_VPMULLD,
  XED_ICLASS_VPMULLW,
  XED_ICLASS_VPMULUDQ,
  XED_ICLASS_VPOR,
  XED_ICLASS_VPSADBW,
  XED_ICLASS_VPSHUFB,
  XED_ICLASS_VPSHUFD,
  XED_ICLASS_VPSHUFHW,
  XED_ICLASS_VPSHUFLW,
  XED_ICLASS_VPSIGNB,
  XED_ICLASS_VPSIGND,
  XED_ICLASS_VPSIGNW,
  XED_ICLASS_VPSLLD,
  XED_ICLASS_VPSLLDQ,
  XED_ICLASS_VPSLLQ,
  XED_ICLASS_VPSLLVD,
  XED_ICLASS_VPSLLVQ,
  XED_ICLASS_VPSLLW,
  XED_ICLASS_VPSRAD,
  XED_ICLASS_VPSRAVD,
  XED_ICLASS_VPSRAW,
  XED_ICLASS_VPSRLD,
  XED_ICLASS_VPSRLDQ,
  XED_ICLASS_VPSRLQ,
  XED_ICLASS_VPSRLVD,
  XED_ICLASS_VPSRLVQ,
  XED_ICLASS_VPSRLW,
  XED_ICLASS_VPSUBB,
  XED_ICLASS_VPSUBD,
  XED_ICLASS_VPSUBQ,
  XED_ICLASS_VPSUBSB,
  XED_ICLASS_VPSUBSW,
  XED_ICLASS_VPSUBUSB,
  XED_ICLASS_VPSUBUSW,
  XED_ICLASS_VPSUBW,
  XED_ICLASS_VPTEST,
  XED_ICLASS_VPUNPCKHBW,
  XED_ICLASS_VPUNPCKHDQ,
  XED_ICLASS_VPUNPCKHQDQ,
  XED_ICLASS_VPUNPCKHWD,
  XED_ICLASS_VPUNPCKLBW,
  XED_ICLASS_VPUNPCKLDQ,
  XED_ICLASS_VPUNPCKLQDQ,
  XED_ICLASS_VPUNPCKLWD,
  XED_ICLASS_VPXOR,
  XED_ICLASS_VRCPPS,
  XED_ICLASS_VRCPSS,
  XED_ICLASS_VROUNDPD,
  XED_ICLASS_VROUNDPS,
  XED_ICLASS_VROUNDSD,
  XED_ICLASS_VROUNDSS,
  XED_ICLASS_VRSQRTPS,
  XED_ICLASS_VRSQRTSS,
  XED_ICLASS_VSHUFPD,
  XED_ICLASS_VSHUFPS,
  XED_ICLASS_VSQRTPD,
  XED_ICLASS_VSQRTPS,
  XED_ICLASS_VSQRTSD,
  XED_ICLASS_VSQRTSS,
  XED_ICLASS_VSTMXCSR,
  XED_ICLASS_VSUBPD,
  XED_ICLASS_VSUBPS,
  XED_ICLASS_VSUBSD,
  XED_ICLASS_VSUBSS,
  XED_ICLASS_VTESTPD,
  XED_ICLASS_VTESTPS,
  XED_ICLASS_VUCOMISD,
  XED_ICLASS_VUCOMISS,
  XED_ICLASS_VUNPCKHPD,
  XED_ICLASS_VUNPCKHPS,
  XED_ICLASS_VUNPCKLPD,
  XED_ICLASS_VUNPCKLPS,
  XED_ICLASS_VXORPD,
  XED_ICLASS_VXORPS,
  XED_ICLASS_VZEROALL,
  XED_ICLASS_VZEROUPPER,
  XED_ICLASS_WBINVD,
  XED_ICLASS_WRFSBASE,
  XED_ICLASS_WRGSBASE,
  XED_ICLASS_WRMSR,
  XED_ICLASS_XABORT,
  XED_ICLASS_XADD,
  XED_ICLASS_XBEGIN,
  XED_ICLASS_XCHG,
  XED_ICLASS_XEND,
  XED_ICLASS_XGETBV,
  XED_ICLASS_XLAT,
  XED_ICLASS_XOR,
  XED_ICLASS_XORPD,
  XED_ICLASS_XORPS,
  XED_ICLASS_XRSTOR,
  XED_ICLASS_XRSTOR64,
  XED_ICLASS_XSAVE,
  XED_ICLASS_XSAVE64,
  XED_ICLASS_XSAVEOPT,
  XED_ICLASS_XSAVEOPT64,
  XED_ICLASS_XSETBV,
  XED_ICLASS_XTEST,
  XED_ICLASS_LAST
} xed_iclass_enum_t;

/// This converts strings to #xed_iclass_enum_t types.
/// @param s A C-string.
/// @return #xed_iclass_enum_t
/// @ingroup ENUM
XED_DLL_EXPORT xed_iclass_enum_t str2xed_iclass_enum_t(const char* s);
/// This converts strings to #xed_iclass_enum_t types.
/// @param p An enumeration element of type xed_iclass_enum_t.
/// @return string
/// @ingroup ENUM
XED_DLL_EXPORT const char* xed_iclass_enum_t2str(const xed_iclass_enum_t p);

/// Returns the last element of the enumeration
/// @return xed_iclass_enum_t The last element of the enumeration.
/// @ingroup ENUM
XED_DLL_EXPORT xed_iclass_enum_t xed_iclass_enum_t_last(void);
#endif
