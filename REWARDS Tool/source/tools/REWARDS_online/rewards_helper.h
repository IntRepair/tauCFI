/**********************************************************************************************
*      This file is part of REWARDS, A Data Structure Reverse Engineering System.             *
*                                                                                             *
*      REWARDS is copyright (C) by Lab FRIENDS at Purdue University (friends@cs.purdue.edu)   *
*      All rights reserved.                                                                   *
*      Do not copy, disclose, or distribute without explicit written                          *
*      permission.                                                                            *
*                                                                                             *
*      Author: Zhiqiang Lin <zlin@cs.purdue.edu>                                              *
**********************************************************************************************/
#ifndef __DYNAMIC_UNIFY_H_
#define __DYNAMIC_UNIFY_H_

#include "pin.H"

VOID MovI2M (THREADID tid, ADDRINT eip, ADDRINT imm, ADDRINT addr, ADDRINT size);
VOID MovR2M (THREADID tid, ADDRINT eip, ADDRINT reg, ADDRINT addr, ADDRINT size);
VOID MovM2M (THREADID tid, ADDRINT eip, ADDRINT addr0, ADDRINT size0, ADDRINT addr1, ADDRINT size1);
VOID MovI2R (THREADID tid, ADDRINT eip, ADDRINT imm, ADDRINT reg);
VOID MovR2R (THREADID tid, ADDRINT eip, ADDRINT reg0, ADDRINT reg1);
VOID MovM2R (THREADID tid, ADDRINT eip, ADDRINT addr, ADDRINT size, ADDRINT reg);
VOID UnionRR(THREADID tid, ADDRINT eip, ADDRINT reg0, ADDRINT reg1, ADDRINT reltype);
VOID UnionRM(THREADID tid, ADDRINT eip, ADDRINT reg, ADDRINT addr, ADDRINT size, ADDRINT reltype);
VOID UnionMR(THREADID tid, ADDRINT eip, ADDRINT addr, ADDRINT size, ADDRINT reg, ADDRINT reltype);
VOID ResolveM(THREADID tid, ADDRINT eip, ADDRINT addr, ADDRINT size, ADDRINT type);
VOID ResolveR(THREADID tid, ADDRINT eip, ADDRINT reg, ADDRINT type);

VOID PushEBP(THREADID tid, ADDRINT eip);
VOID SetESP (ADDRINT pc, ADDRINT esp);

VOID ClrStack(ADDRINT addr, ADDRINT size);
VOID Call(ADDRINT eip, ADDRINT target, ADDRINT esp, ADDRINT ebp, ADDRINT retaddr);
VOID Ret (ADDRINT eip, ADDRINT esp, ADDRINT eax);
VOID ResolveFunctionParameter(ADDRINT pc, ADDRINT addr, ADDRINT ebp);
VOID ResolveFunctionRegParameter(ADDRINT pc, ADDRINT reg, ADDRINT reg_value);
VOID ResolveFunctionReturn(ADDRINT pc);

VOID PIN_FAST_ANALYSIS_CALL
ResolvePointer(THREADID tid, ADDRINT pc, ADDRINT reg, ADDRINT addr, ADDRINT size);

#endif
