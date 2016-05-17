/**********************************************************************************************
*      This file is part of REWARDS, A Data Structure Reverse Engineering System.             *
*                                                                                             *
*      REWARDS is copyright (C) by Lab FRIENDS at Purdue University (friends@cs.purdue.edu)   *
*      All rights reserved.                                                                   *
*      Do not copy, disclose, or distribute without explicit written                          *
*      permission.                                                                            *
*                                                                                             *
*      Author: Zhiqiang Lin <zlin@cs.purdue.edu>                                              *
*********************************************************************************************/
#include <stdlib.h>
#include "inttypes.h"
#include "rewards.h"
#include "rewards_helper.h"
#include "tls.h"
#include "tag.h"
#include "assert.h"

//#define UNKNOWN_TAG 1
#define DATA_UNDEFINED   0xFFFFFFFF

uint32_t time_stamp_g;
uint32_t simulated_pc;

VOID MovI2M(THREADID tid, ADDRINT pc, ADDRINT srcimm, ADDRINT dstaddr, ADDRINT dstsize)
{
    SetMTAG(dstaddr, dstsize, NewTAG(pc, srcimm), pc);
}

VOID MovR2M(THREADID tid, ADDRINT pc, ADDRINT srcreg, ADDRINT dstaddr, ADDRINT dstsize)
{
    SetMTAG(dstaddr, dstsize, GetOrNewRTAG((REG)srcreg, pc), pc);
}

VOID MovM2M(THREADID tid, ADDRINT pc, ADDRINT srcaddr, ADDRINT srcsize, ADDRINT dstaddr, ADDRINT dstsize)
{
    SetMTAG(dstaddr, dstsize, GetOrNewMTAG(srcaddr, srcsize, pc), pc);
}

VOID MovI2R(THREADID tid, ADDRINT pc, ADDRINT srcimm, ADDRINT dstreg)
{
    SetRTAG((REG)dstreg, NewTAG(pc, srcimm));
}

VOID MovR2R(THREADID tid, ADDRINT pc, ADDRINT srcreg, ADDRINT dstreg)
{
    SetRTAG((REG)dstreg, GetOrNewRTAG((REG)srcreg, pc));
}

VOID MovM2R(THREADID tid, ADDRINT pc, ADDRINT srcaddr, ADDRINT srcsize, ADDRINT dstreg)
{
    SetRTAG((REG)dstreg, GetOrNewMTAG(srcaddr, srcsize, pc));
}

VOID UnionRR(THREADID tid, ADDRINT pc, ADDRINT srcreg, ADDRINT dstreg, ADDRINT reltype)
{
    SetRTAG((REG)dstreg,
        UnionTAG( pc, GetOrNewRTAG((REG)dstreg, pc), GetOrNewRTAG((REG)srcreg, pc), (REL_TYPE)reltype ));
}

VOID UnionRM(THREADID tid, ADDRINT pc, ADDRINT srcreg, ADDRINT dstaddr, ADDRINT dstsize, ADDRINT reltype)
{
    SetMTAG(dstaddr, dstsize,
        UnionTAG( pc, GetOrNewMTAG(dstaddr, dstsize, pc), GetOrNewRTAG((REG)srcreg, pc), (REL_TYPE)reltype ), pc);
}

VOID UnionMR(THREADID tid, ADDRINT pc, ADDRINT srcaddr, ADDRINT srcsize, ADDRINT dstreg, ADDRINT reltype)
{
    SetRTAG((REG)dstreg,
        UnionTAG( pc, GetOrNewRTAG((REG)dstreg, pc), GetOrNewMTAG(srcaddr, srcsize, pc), (REL_TYPE)reltype ));
}

VOID ResolveM(THREADID tid, ADDRINT pc, ADDRINT addr, ADDRINT size, ADDRINT type)
{
    ResolveTAG( GetOrNewMTAG(addr, size, pc), (T_TYPE)type );
}

VOID ResolveR(THREADID tid, ADDRINT pc, ADDRINT reg, ADDRINT type)
{
    ResolveTAG( GetOrNewRTAG((REG)reg, pc), (T_TYPE)type );
}


VOID SetESP (ADDRINT pc, ADDRINT esp)
{
    TAG tag = NewTAG(esp);
    SetRTAG( REG_ESP, tag );
    ResolveTAG( tag, T_STACK_PTR );
}

VOID PushEBP(THREADID tid, ADDRINT pc)
{
    ResolveTAG( GetOrNewRTAG(REG_EBP, pc), T_EBP ); 
}

VOID ClrStack(ADDRINT addr, ADDRINT size)
{
    ClrTAG(addr-size, size);
}

VOID Call(ADDRINT eip, ADDRINT target, ADDRINT esp, ADDRINT ebp, ADDRINT retaddr)
{
    TLS tls = GetTLS();
    if (!IsInCodeSpace(target)) return;

    ClrTAG(esp-1000, 1000);

    TAG tag = NewTAG(eip);
    SetMTAG( esp-4, 4, tag, eip );
    ResolveTAG( tag, T_RET_ADDR );

    CALLENTRY entry;
    entry.callsite = eip;
    entry.funaddr = target;
    entry.esp = esp - 4;
    entry.param0 = esp;
    if (!tls->callstack.empty())
        entry.param1 = tls->callstack.back().param0;
    else
        entry.param1 = ebp;
    entry.retaddr = retaddr;
    entry.regparam = 1;
    entry.nparam = 0;

    assert (entry.param0 <= entry.param1);

    if (!tls->callstack.empty()) {
        tls->callstack.back().regparam = 0;
    }

    tls->callstack.push_back(entry);

    UpdateTimeStamp();

    if (IsInCodeSpace(target)) {
        BEGIN_PRINT();

#define NPARAM 30 
        TAG param[NPARAM];
        ADDRINT value[NPARAM];

        for (int i = 0; i < NPARAM; ++i) {
            param[i] = GetMTAG(esp + 4*i, 4, eip);
            value[i] = 0;
            PIN_SafeCopy(&value[i], (VOID*)(esp + 4*i), 4);
        }

        PRINT ("T %08x %08x", target, esp);

        for (int i = 0; i < NPARAM; ++i) {
            PRINT (" %08x %08x", param[i], value[i]);
        }

        PRINT ("\n");

        END_PRINT();
    }

    tls->regRTN[REG_EAX] = tls->regRTN[REG_AX] =
    tls->regRTN[REG_AH] = tls->regRTN[REG_AL] = 0;
    tls->retfunction = 0;
    tls->exittag = 0;
    tls->exitvalue = 0;
}

VOID Ret (ADDRINT pc, ADDRINT esp, ADDRINT eax)
{
    TLS tls = GetTLS();

    if (!tls->callstack.empty() && tls->callstack.back().retaddr == pc) {

        CALLENTRY& entry = tls->callstack.back();

//        if (IsInCodeSpace(entry.funaddr)) {
//            BEGIN_PRINT();
//            PRINT ("R %08x %08x %08x\n", entry.funaddr, GetRTAG(REG_EAX, pc), tls->exitvalue);
//            END_PRINT();
//        }

        tls->exittag = GetRTAG(REG_EAX, pc);
        tls->exitvalue = eax;
        tls->retfunction = entry.funaddr;

        DEBUG("Clear params : %d\n", entry.nparam);
        ClrTAG(esp, 4);
        ClrTAG(esp+4, 4 * entry.nparam);

        tls->callstack.pop_back();
    }

    ClrTAG(esp-1000, 1000);
}

VOID ResolveFunctionParameter (ADDRINT pc, ADDRINT addr, ADDRINT ebp)
{
    TLS tls = GetTLS();

    if (tls->callstack.empty()) return;
//    if (addr <= ebp) return;

    CALLENTRY& entry = tls->callstack.back();
    if (addr >= entry.param0 && addr <= entry.param1) {
        ADDRINT value;

        BEGIN_PRINT();
        PIN_SafeCopy(&value, (VOID*)addr, 4);
        PRINT ("C %08x %08x %08x %08x %08x\n", entry.funaddr, addr - entry.param0, GetMTAG(addr, 4, pc), value, pc);
        END_PRINT();

        entry.nparam = max(entry.nparam, (addr - entry.param0) / 4 + 1);
    }
}

VOID ResolveFunctionRegParameter (ADDRINT pc, ADDRINT reg, ADDRINT reg_value)
{
    TLS tls = GetTLS();
    TAG tag = tls->regTagMap[reg];
    ADDRINT rtn = tls->regRTN[reg];

    if (tls->callstack.empty()) return;
    if (!tag) return;

    CALLENTRY& entry = tls->callstack.back();

    if (entry.funaddr != rtn && entry.regparam == 1) {
        int offset = 0;
        switch (reg) {
        case REG_EAX: case REG_AX: case REG_AH: case REG_AL: offset = 1; break;
        case REG_EBX: case REG_BX: case REG_BH: case REG_BL: offset = 2; break;
        case REG_ECX: case REG_CX: case REG_CH: case REG_CL: offset = 3; break;
        case REG_EDX: case REG_DX: case REG_DH: case REG_DL: offset = 4; break;
        default: break;
        }

        if (offset == 1 && rtn != 0)
            return;

        BEGIN_PRINT();
        PRINT("D %08x %08x %08x %08x %08x\n", entry.funaddr, offset, tag, reg_value, pc);
        END_PRINT();
    }
}

VOID ResolveFunctionReturn (ADDRINT pc)
{
    TLS tls = GetTLS();

    if (tls->retfunction == 0) return;
    if (tls->callstack.empty()) return;
    if (!IsInCodeSpace(tls->retfunction)) return;

    CALLENTRY& entry = tls->callstack.back();

    if (entry.funaddr != tls->regRTN[REG_EAX]) {
        BEGIN_PRINT();
        PRINT("R %08x %08x %08x %08x\n", tls->retfunction, tls->exittag, tls->exitvalue, pc);
        END_PRINT();
    }
}

VOID PIN_FAST_ANALYSIS_CALL
ResolvePointer (THREADID tid, ADDRINT pc, ADDRINT reg, ADDRINT addr, ADDRINT size)
{
    size = size / 8;
    TAG rtag = GetOrNewRTAG((REG)reg, pc);
    TAG mtag = GetOrNewMTAG(addr, size, pc);
     
    SetRTAG((REG)reg, PtrATAG(pc, rtag, mtag));
    SetMTAG(addr, size, PtrBTAG(pc, rtag, mtag), pc);
}
