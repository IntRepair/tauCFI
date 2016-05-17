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

#include "pin.H"
#include "inttypes.h"
#include "rewards.h"
#include "rewards_helper.h"
#include "types.h"
#include "tag.h"

// Bitmasks for accessing specific parts of eflags
#define CF_MASK (1 << 1)        // bit 1
#define PF_MASK (1 << 2)        // bit 2
#define AF_MASK (1 << 4)        // bit 4
#define ZF_MASK (1 << 6)        // bit 6
#define SF_MASK (1 << 7)        // bit 7
#define OF_MASK (1 << 11)       // bit 11

#define INS_RR(fn, reg0, reg1) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_ADDRINT, (ADDRINT)(reg0), \
                    IARG_ADDRINT, (ADDRINT)(reg1), \
                    IARG_END)

#define INS_IR(fn, imm, reg) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_ADDRINT, (ADDRINT)(imm), \
                    IARG_ADDRINT, (ADDRINT)(reg), \
                    IARG_END)

#define INS_MR(fn, reg) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, \
                    IARG_ADDRINT, (ADDRINT)(reg), \
                    IARG_END)

#define INS_RM(fn, reg) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_ADDRINT, (ADDRINT)(reg), \
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, \
                    IARG_END)

#define INS_IM(fn, imm) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_ADDRINT, (ADDRINT)(imm), \
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, \
                    IARG_END)

#define INS_MM(fn) \
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(fn), \
                    IARG_THREAD_ID, IARG_INST_PTR,   \
                    IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, \
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, \
                    IARG_END)

#define A_ResolveFunctionParameter() do { \
        if (INS_IsMemoryRead(ins) && IsInCodeSpace(INS_Address(ins)) && (INS_MemoryBaseReg(ins) == REG_EBP || INS_MemoryBaseReg(ins) == REG_ESP)) { \
            INS_InsertCall(ins, IPOINT_BEFORE,AFUNPTR(ResolveFunctionParameter), \
                IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_REG_VALUE, REG_EBP, IARG_END);  \
        } } while(0)


static void UnimplementedInstruction(INS ins, void *v)
{
    ERROR ("%s unimplemented[%d]\n",
               INS_Disassemble(ins).c_str(), INS_Opcode(ins));
}

//CMOVcc check functions
ADDRINT CheckCMOVNB(ADDRINT eflags)
{
    return ((eflags & CF_MASK) == 0);
}

ADDRINT CheckCMOVB(ADDRINT eflags)
{
    return ((eflags & CF_MASK) == 1);
}

ADDRINT CheckCMOVBE(ADDRINT eflags)
{
    return (((eflags & CF_MASK) == 1) || ((eflags & ZF_MASK) == 1));
}

ADDRINT CheckCMOVNLE(ADDRINT eflags)
{
    return ((eflags & ZF_MASK) == 0) && ((eflags & SF_MASK) == 0);
}

ADDRINT CheckCMOVNL(ADDRINT eflags)
{
    return ((eflags & SF_MASK) == (eflags & OF_MASK));
}

ADDRINT CheckCMOVL(ADDRINT eflags)
{
    return ((eflags & SF_MASK) != (eflags & OF_MASK));
}

ADDRINT CheckCMOVLE(ADDRINT eflags)
{
    return (((eflags & ZF_MASK) == 1)
            || ((eflags & SF_MASK) != (eflags & OF_MASK)));
}

ADDRINT CheckCMOVNBE(ADDRINT eflags)
{
    return ((eflags & CF_MASK) == 0) && ((eflags & ZF_MASK) == 0);
}

ADDRINT CheckCMOVNZ(ADDRINT eflags)
{
    return ((eflags & ZF_MASK) == 0);
}

ADDRINT CheckCMOVNO(ADDRINT eflags)
{
    return ((eflags & OF_MASK) == 0);
}

ADDRINT CheckCMOVNP(ADDRINT eflags)
{
    return ((eflags & PF_MASK) == 0);
}

ADDRINT CheckCMOVNS(ADDRINT eflags)
{
    return ((eflags & SF_MASK) == 0);
}

ADDRINT CheckCMOVO(ADDRINT eflags)
{
    return ((eflags & OF_MASK) == 1);
}

ADDRINT CheckCMOVP(ADDRINT eflags)
{
    return ((eflags & PF_MASK) == 1);
}

ADDRINT CheckCMOVS(ADDRINT eflags)
{
    return ((eflags & SF_MASK) == 1);
}

ADDRINT CheckCMOVZ(ADDRINT eflags)
{
    return ((eflags & ZF_MASK) == 1);
}

//CMPXCHG check functions
ADDRINT CheckEqual_r_r(ADDRINT v1, ADDRINT v2)
{
    return v1 == v2;
}

ADDRINT CheckNotEqual_r_r(ADDRINT v1, ADDRINT v2)
{
    return v1 != v2;
}

ADDRINT CheckEqual_m_r(ADDRINT start, ADDRINT size, ADDRINT v2)
{
    ADDRINT v1 = *((ADDRINT *) start);
    return v1 == v2;
}

ADDRINT CheckNotEqual_m_r(ADDRINT start, ADDRINT size, ADDRINT v2)
{
    ADDRINT v1 = *((ADDRINT *) start);
    return v1 == v2;
}

static void INS_Union(INS ins, REL_TYPE reltype)
{
    for (int i = 0; i < 2; i++) {
        if (!INS_OperandIsReg(ins, i)) continue;

        REG reg = INS_OperandReg(ins, i);

        if (IsInCodeSpace(INS_Address(ins)) && 
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL ||
                 reg == REG_ECX || reg == REG_CX || reg == REG_CH || reg == REG_CL ||
                 reg == REG_EDX || reg == REG_DX || reg == REG_DH || reg == REG_DL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionRegParameter),
                IARG_INST_PTR, IARG_ADDRINT, reg, IARG_REG_VALUE, reg, IARG_END);
        } 

        if (IsInCodeSpace(INS_Address(ins)) && 
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionReturn), IARG_INST_PTR, IARG_END);
        }
    }

    A_ResolveFunctionParameter();

//0. any update to ESP register
    if(INS_OperandIsReg(ins, 0))
    {
        if(INS_OperandReg(ins, 0) == REG_ESP)
        {
            if (INS_HasFallThrough(ins)) {
                INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(SetESP),
                               IARG_INST_PTR,
                               IARG_REG_VALUE, REG_ESP, IARG_END);
            }
        }
    }

//1. R + X -> R
    if (INS_OperandIsReg(ins, 0)) {
        ///////////////////////////////////////////////////////////////////////////////
        //dest <- src
        if (INS_OperandIsReg(ins, 1)) {
            INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(UnionRR),
                            IARG_THREAD_ID, IARG_INST_PTR,
                            IARG_ADDRINT, (ADDRINT)(INS_OperandReg(ins, 1)),
                            IARG_ADDRINT, (ADDRINT)(INS_OperandReg(ins, 0)),
                            IARG_ADDRINT, (ADDRINT)reltype,
                            IARG_END);

        } else if (INS_OperandIsMemory(ins, 1)) {
            if (!INS_IsMemoryRead(ins))
                return;

            INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(UnionMR),
                            IARG_THREAD_ID, IARG_INST_PTR,
                            IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
                            IARG_ADDRINT, (ADDRINT)(INS_OperandReg(ins, 0)),
                            IARG_ADDRINT, (ADDRINT)reltype,
                            IARG_END);

        } else if (INS_OperandIsImmediate(ins, 1)) {
            //nothing
        } else {
            ERROR ("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
        }
        //////////////////////////////////////////////////////////////////////////////////
    }
//2. M + X -> M
    else if (INS_OperandIsMemory(ins, 0)) {
        ///////////////////////////////////////////////////////////////////////////////
        //dest <- src
        if (INS_OperandIsReg(ins, 1)) {
            INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(UnionRM),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_ADDRINT, (ADDRINT)(INS_OperandReg(ins, 1)),
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                    IARG_ADDRINT, (ADDRINT)reltype,
                    IARG_END);

        } else if (INS_OperandIsMemory(ins, 1)) {
            ERROR ("Memory binop Memory: %s\n", INS_Disassemble(ins).c_str());

        } else if (INS_OperandIsImmediate(ins, 1)) {
            //nothing
        } else {
            ERROR ("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
        }
        //////////////////////////////////////////////////////////////////////////////////

    } else if (INS_OperandIsImmediate(ins, 0)) {
        ERROR ("Impossible happens. first operand in binop is immeidate value\n");
    }
}

//static void Instrument_ADD(INS ins, void *v);
static void Instrument_ADC(INS ins, void *v)
{
    INS_Union(ins, REL_ADD);
}

// This is a good example of how instructions are modeled and taint is
// propagated.

// To implement one of these functions the first thing to do is decide
// how taint marks should be propagated.

// In this instance, for ADD, the destination operand and the eflags register
// are tainted with the union of the taint marks associated with the
// destination and source operands.

// dest, eflags <- union(dest, src)

// Then the general implementation goes like this
// 1. load the taint marks associated with the operands on the right hand
//    side of the equation (dest, src)
// 2. figure out the union
// 3. assign the union to the operands on the left hand side (dest, eflags)

static void Instrument_ADD(INS ins, void *v)
{
    INS_Union(ins, REL_ADD);
}

static void Instrument_AND(INS ins, void *v)
{
    if (INS_OperandIsImmediate(ins, 1) && INS_OperandImmediate(ins, 1) == (UINT64)0) {
        if (INS_OperandIsMemory(ins, 0)) {
            INS_IM (MovI2M, 0);
        } else if (INS_OperandIsReg(ins, 0)) {
            INS_IR (MovI2R, 0, INS_OperandReg(ins, 0));
        }
    }
    else  {
        INS_Union(ins, REL_COMBINE);
    }
}

static void Instrument_BSWAP(INS ins, void *v)
{
//pass
}

static void Instrument_CALL_FAR(INS ins, void *v)
{
}

static void Instrument_CALL_NEAR(INS ins, void *v)
{
//TODO: Function pointer call eax;
//pass
    if (INS_OperandIsReg(ins, 0)) {
        INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(ResolveR),
                        IARG_THREAD_ID, IARG_INST_PTR, 
                        IARG_ADDRINT, INS_OperandReg(ins, 0),
                        IARG_ADDRINT, (ADDRINT)T_FUNCPTR,
                        IARG_END);
    }
//	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(UpdateTimeStamp), IARG_END);
}

static void Instrument_CDQ(INS ins, void *v)
{
}

static void Instrument_CLD(INS ins, void *v)
{
}

static void Instrument_CMOVcc(INS ins, void *v)
{
}

static void Instrument_CMP(INS ins, void *v)
{
    for (int i = 0; i < 2; i++) {
        if (!INS_OperandIsReg(ins, i)) continue;

        REG reg = INS_OperandReg(ins, i);

        if (IsInCodeSpace(INS_Address(ins)) && 
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL ||
                 reg == REG_ECX || reg == REG_CX || reg == REG_CH || reg == REG_CL ||
                 reg == REG_EDX || reg == REG_DX || reg == REG_DH || reg == REG_DL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionRegParameter),
                IARG_INST_PTR, IARG_ADDRINT, reg, IARG_REG_VALUE, reg, IARG_END);
        } 

        if (IsInCodeSpace(INS_Address(ins)) && 
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionReturn), IARG_INST_PTR, IARG_END);
        }
    }

    A_ResolveFunctionParameter();

    return;

//  Instrument_ADD(ins, v);
    if (INS_OperandIsImmediate(ins, 0) || INS_OperandIsImmediate(ins, 1)) {
        return;		//because add eax, 0x10 will not modify eax taint tag
    }


}

static void Instrument_CMPSB(INS ins, void *v)
{
}

static void Instrument_CMPXCHG(INS ins, void *v)
{
}

static void Instrument_CWDE(INS ins, void *v)
{
}

static void Instrument_DEC(INS ins, void *v)
{
}

static void Instrument_DIV(INS ins, void *v)
{
//    Instrument_ADD(ins, v);
}

static void Instrument_FLDCW(INS ins, void *v)
{   //TODO: Floating point instruction
//pass
}

static void Instrument_FNSTCW(INS ins, void *v)
{   /*
       INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
       IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
       IARG_END);
     */
}

static void Instrument_HLT(INS ins, void *v)
{
//pass
}

static void Instrument_IDIV(INS ins, void *v)
{

}

static void Instrument_IMUL(INS ins, void *v)
{

}

static void Instrument_INC(INS ins, void *v)
{
//no changement with the taint tag
}

static void Instrument_INT(INS ins, void *v)
{
    //pass
}

static void Instrument_Jcc(INS ins, void *v)
{
    //TODO
}

static void Instrument_JMP(INS ins, void *v)
{
    //PASS
}

static void Instrument_LEA(INS ins, void *v)
{

//  Instrument_ADD(ins, v);
//  INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(CheckLeaAddr),
    if (INS_HasFallThrough(ins)) {
        INS_InsertCall (ins, IPOINT_AFTER, AFUNPTR(MovI2R),
                        IARG_THREAD_ID, IARG_INST_PTR,
                        IARG_REG_VALUE, INS_OperandReg(ins, 0),
                        IARG_ADDRINT, INS_OperandReg(ins, 0),
                        IARG_END);

        INS_InsertCall (ins, IPOINT_AFTER, AFUNPTR(ResolvePointer),
                        IARG_THREAD_ID, IARG_INST_PTR,
                        IARG_ADDRINT, INS_OperandReg(ins, 0),
                        IARG_REG_VALUE, INS_OperandReg(ins, 0),
                        IARG_ADDRINT, 0,
                        IARG_FAST_ANALYSIS_CALL,
                        IARG_END);

// FOR LEA, we just need to set the lea eax, 0xxxx
// with a pointer tag
        /*
        	  INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(InitializeTaintPointerForRegister),
                   IARG_ADDRINT, INS_OperandReg(ins, 0),
        		   IARG_UINT32,  T_POINTER,
                   IARG_REG_VALUE, INS_OperandReg(ins, 0),
        		   IARG_END);
        */
    }

}

static void Instrument_LEAVE(INS ins, void *v)
{   //Stack variable life time ends
}

static void Instrument_LDMXCSR(INS ins, void *v)
{
//float point instruction
//pass
}

static void Instrument_MOV(INS ins, void *v)
{
//1. R -> R | M
    if (INS_OperandIsReg(ins, 1)) {
        ///////////////////////////////////////////////////////////////////////////////
        //dest <- src
        if (INS_OperandIsReg(ins, 0)) {
            INS_RR (MovR2R, INS_OperandReg(ins, 1), INS_OperandReg(ins, 0));

        } else if (INS_OperandIsMemory(ins, 0)) {
            if (!INS_IsMemoryWrite(ins))
                return;
            INS_RM (MovR2M, INS_OperandReg(ins, 1));

        } else {
            ERROR ("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
        }
        //////////////////////////////////////////////////////////////////////////////////

        REG reg = INS_OperandReg(ins, 1);

        if (IsInCodeSpace(INS_Address(ins)) && 
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL ||
                 reg == REG_ECX || reg == REG_CX || reg == REG_CH || reg == REG_CL ||
                 reg == REG_EDX || reg == REG_DX || reg == REG_DH || reg == REG_DL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionRegParameter),
                IARG_INST_PTR, IARG_ADDRINT, reg, IARG_REG_VALUE, reg, IARG_END);
        } 

        if (IsInCodeSpace(INS_Address(ins)) &&
                (reg == REG_EAX || reg == REG_AX || reg == REG_AH || reg == REG_AL)) {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveFunctionReturn), IARG_INST_PTR, IARG_END);
        }
    }
//2. M -> R| M
    else if (INS_OperandIsMemory(ins, 1)) {
        if (!INS_IsMemoryRead(ins))
            return;
        ///////////////////////////////////////////////////////////////////////////////
        //dest <- src
        if (INS_OperandIsReg(ins, 0)) {
            INS_MR (MovM2R, INS_OperandReg(ins, 0));

        } else if (INS_OperandIsMemory(ins, 0)) {
            if (!INS_IsMemoryWrite(ins))
                return;
            INS_MM (MovM2M);

        } else {
            ERROR ("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
        }
        //////////////////////////////////////////////////////////////////////////////////

        A_ResolveFunctionParameter();
    }
//3. Imm -> R | M
    else if (INS_OperandIsImmediate(ins, 1)) {
        ///////////////////////////////////////////////////////////////////////////////
        //dest <- src
        if (INS_OperandIsReg(ins, 0)) {
            INS_IR (MovI2R, INS_OperandImmediate(ins, 1), INS_OperandReg(ins, 0));

        } else if (INS_OperandIsMemory(ins, 0)) {
            if (!INS_IsMemoryWrite(ins))
                return;
            INS_IM (MovI2M, INS_OperandImmediate(ins, 1));

        } else {
            ERROR ("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
        }
        //////////////////////////////////////////////////////////////////////////////////
    } else {
        ERROR ("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    }
}

static void Instrument_MOVSB(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovM2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_MEMORYREAD_EA, IARG_ADDRINT, 1 /*IARG_MEMORYREAD_SIZE*/,
                    IARG_MEMORYWRITE_EA, IARG_ADDRINT, 1 /*IARG_MEMORYWRITE_SIZE*/,
                    IARG_END);

    A_ResolveFunctionParameter();
}

static void Instrument_MOVSD(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovM2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_MEMORYREAD_EA, IARG_ADDRINT, 4 /*IARG_MEMORYREAD_SIZE*/,
                    IARG_MEMORYWRITE_EA, IARG_ADDRINT, 4 /*IARG_MEMORYWRITE_SIZE*/,
                    IARG_END);

    A_ResolveFunctionParameter();
}

static void Instrument_MOVSW(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovM2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_MEMORYREAD_EA, IARG_ADDRINT, 2 /*IARG_MEMORYREAD_SIZE*/,
                    IARG_MEMORYWRITE_EA, IARG_ADDRINT, 2 /*IARG_MEMORYWRITE_SIZE*/,
                    IARG_END);

    A_ResolveFunctionParameter();
}

static void Instrument_MOVSX(INS ins, void *v)
{
    Instrument_MOV(ins, v);
}

static void Instrument_MOVZX(INS ins, void *v)
{   // movzx r/m, r
    Instrument_MOV(ins, v);
}

static void Instrument_MUL(INS ins, void *v)
{
//    Instrument_ADD(ins, v);
}

static void Instrument_NEG(INS ins, void *v)
{
}

static void Instrument_NOT(INS ins, void *v)
{
//pass
}

static void Instrument_OR(INS ins, void *v)
{
    if (INS_OperandIsImmediate(ins, 1) && INS_OperandImmediate(ins, 1) == (UINT64)-1) {
        if (INS_OperandIsMemory(ins, 0)) {
            INS_IM (MovI2M, 0xffffffff); 

        } else if (INS_OperandIsReg(ins, 0)) {
            INS_IR (MovI2R, 0xffffffff, INS_OperandReg(ins, 0));
        }
    }
    else {
        INS_Union(ins, REL_COMBINE);
    }
}

static void Instrument_PAUSE(INS ins, void *v)
{
//pass
}

static void Instrument_POP(INS ins, void *v)
{
    if (INS_OperandIsReg(ins, 0)) {
        INS_MR (MovM2R, INS_OperandReg(ins, 0));

    } else if (INS_OperandIsMemory(ins, 0)) {
        INS_MM (MovM2M);

    } else if (INS_OperandIsImmediate(ins, 0)) {
        ERROR ("Impossible happen, destination is immediate\n");
    }
}

static void Instrument_POPFD(INS ins, void *v)
{
}

static void Instrument_PUSH(INS ins, void *v)
{
    if (INS_OperandIsReg(ins, 0)) {
//        if  (INS_OperandReg(ins, 0) == REG_EBP)
//            INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(PushEBP),
//                            IARG_THREAD_ID, IARG_INST_PTR,
//                            IARG_END);

        INS_RM (MovR2M, INS_OperandReg(ins, 0));

    } else if (INS_OperandIsImmediate(ins, 0)) {
        INS_IM (MovI2M, INS_OperandImmediate(ins, 0));

    } else if (INS_OperandIsMemory(ins, 0)) {
        INS_MM (MovM2M);
    }

    INS_InsertCall (ins, IPOINT_AFTER, AFUNPTR(SetESP),
                    IARG_INST_PTR,
                    IARG_REG_VALUE, REG_ESP, IARG_END);
}

static void Instrument_PUSHFD(INS ins, void *v)
{
}
static void Instrument_RDTSC(INS ins, void *v)
{
}

static void Instrument_RET_NEAR(INS ins, void *v)
{
}

static void Instrument_RET_FAR(INS ins, void *v)
{
}

static void Instrument_SAR(INS ins, void *v)
{
}

static void Instrument_SBB(INS ins, void *v)
{
    INS_Union(ins, REL_SUB);
}

static void Instrument_SCASB(INS ins, void *v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveM),
                   IARG_THREAD_ID, IARG_INST_PTR,
                   IARG_MEMORYREAD_EA, IARG_ADDRINT, 1, 
                   IARG_ADDRINT, (ADDRINT)T_CHAR,
                   IARG_END);
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveR),
                   IARG_THREAD_ID, IARG_INST_PTR,
                   IARG_ADDRINT, REG_EDI,
                   IARG_ADDRINT, (ADDRINT)(T_PTR | T_CHAR),
                   IARG_END);

    if (INS_HasRealRep(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveR),
                   IARG_THREAD_ID, IARG_INST_PTR,
                   IARG_ADDRINT, REG_ECX,
                   IARG_ADDRINT, (ADDRINT)(T_UNSIGNED_INT),
                   IARG_END);
    }
}

static void Instrument_SETcc(INS ins, void *v)
{
}

static void Instrument_SHL(INS ins, void *v)
{
}

static void Instrument_SHLD(INS ins, void *v)
{

}

static void Instrument_SHR(INS ins, void *v)
{
    Instrument_SAR(ins, v);
}

static void Instrument_SHRD(INS ins, void *v)
{
}

static void Instrument_STD(INS ins, void *v)
{
}

static void Instrument_STMXCSR(INS ins, void *v)
{
}

static void Instrument_STOSB(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovR2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_ADDRINT, REG_AL,
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                    IARG_END);
}
static void Instrument_STOSW(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovR2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_ADDRINT, REG_AX,
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                    IARG_END);
}

static void Instrument_STOSD(INS ins, void *v)
{
    INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR(MovR2M),
                    IARG_THREAD_ID, IARG_INST_PTR,
                    IARG_ADDRINT, REG_EAX,
                    IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                    IARG_END);
}

static void Instrument_SUB(INS ins, void *v)
{
    INS_Union(ins, REL_SUB);

    if (INS_OperandIsReg(ins, 0) && INS_OperandReg(ins, 0) == REG_ESP) {

        if (INS_OperandIsImmediate(ins, 1)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ClrStack,
                    IARG_REG_VALUE, REG_ESP,
                    IARG_ADDRINT, (ADDRINT)INS_OperandImmediate(ins, 1),
                    IARG_END);
        }

        else if (INS_OperandIsReg(ins, 1)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ClrStack,
                    IARG_REG_VALUE, REG_ESP,
                    IARG_REG_VALUE, INS_OperandReg(ins, 1),
                    IARG_END);
        }
    }
}

static void Instrument_TEST(INS ins, void *v)
{
}

static void Instrument_XADD(INS ins, void *v)
{
}

static void Instrument_XCHG(INS ins, void *v)
{
    return;

#if 0
    if (INS_OperandIsMemory(ins, 0) && INS_OperandIsReg(ins, 1)) {
        if (INS_OperandMemoryBaseReg(ins, 0) == REG_ESP)
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(UpdateCallStackbyXCHG), IARG_MEMORYWRITE_EA,	//memaddr
                           IARG_REG_VALUE, REG_ESP,	//*esp
                           IARG_ADDRINT, INS_OperandReg(ins, 1),	//eax
                           IARG_REG_VALUE, INS_OperandReg(ins, 1),	//*eax
                           IARG_END);
    }
#endif
}

static void Instrument_XOR(INS ins, void *v)
{
    if (INS_OperandIsReg(ins, 0) && INS_OperandIsReg(ins, 1) &&
        INS_OperandReg(ins, 0) == INS_OperandReg(ins, 1)) {

        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(MovI2R),
                           IARG_THREAD_ID, IARG_INST_PTR,
                           IARG_ADDRINT, 0,
                           IARG_ADDRINT, INS_OperandReg(ins, 0),	//eax
                           IARG_END);
    }
    else {
        INS_Union(ins, REL_ADD);
    }
}

/*
static void Instrument_DAA(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);

}

static void Instrument_DAS(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);

}

static void Instrument_AAA(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);

}

static void Instrument_AAS(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);

}

static void Instrument_ROL(INS ins, void *v)
{
}
*/


VOID Instrument_Trace(TRACE trace)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

            if (INS_IsCall(ins)) {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Call),
                        IARG_INST_PTR,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_REG_VALUE, REG_ESP,
                        IARG_REG_VALUE, REG_EBP,
                        IARG_ADDRINT, INS_NextAddress(ins),
                        IARG_CALL_ORDER, CALL_ORDER_LAST,
                        IARG_END);
            }

            if (INS_IsRet(ins)) {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Ret),
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_REG_VALUE, REG_ESP,
                        IARG_REG_VALUE, REG_EAX,
                        IARG_CALL_ORDER, CALL_ORDER_FIRST,
                        IARG_END);
            }
        }
    }
}

void SetupDataflow(INS ins, VOID*);
VOID InstrumentINS(INS ins, VOID* v)
{
    HandleStaticLinkFunction(ins);

    for (UINT32 i = 0; i < INS_MemoryOperandCount(ins); i++) {
        UINT32 opidx = INS_MemoryOperandIndexToOperandIndex(ins, i);
        REG basereg = INS_OperandMemoryBaseReg(ins, opidx);
        if (!REG_valid(basereg)) continue;
        if (basereg == REG_EIP || basereg == REG_ESP || basereg == REG_EBP) continue;

//        if (INS_HasFallThrough(ins))
        INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR)ResolvePointer,
                IARG_THREAD_ID,
                IARG_INST_PTR,
                IARG_ADDRINT, (ADDRINT)basereg,
                IARG_MEMORYOP_EA, i,
                IARG_ADDRINT, (ADDRINT)INS_OperandWidth(ins, opidx),
                IARG_FAST_ANALYSIS_CALL,
                IARG_END);
    }

    SetupDataflow(ins, v);
}


typedef void (*InstrumentFunction) (INS ins, void *v);
InstrumentFunction instrument_functions[XED_ICLASS_LAST];

void SetupDataflow(INS ins, VOID* v)
{
    xed_iclass_enum_t opcode = (xed_iclass_enum_t) INS_Opcode(ins);

    (*instrument_functions[opcode]) (ins, v);
}

void setup_hook()
{
    // set a default handling function that aborts.  This makes sure I don't
    // miss instructions in new applications
    for (int i = 0; i < XED_ICLASS_LAST; i++) {
        instrument_functions[i] = &UnimplementedInstruction;
    }
    instrument_functions[XED_ICLASS_ADD] = &Instrument_ADD;	// 1
    instrument_functions[XED_ICLASS_PUSH] = &Instrument_PUSH;	// 2
    instrument_functions[XED_ICLASS_POP] = &Instrument_POP;	// 3
    instrument_functions[XED_ICLASS_OR] = &Instrument_OR;	// 4

    instrument_functions[XED_ICLASS_ADC] = &Instrument_ADC;	// 6
    instrument_functions[XED_ICLASS_SBB] = &Instrument_SBB;	// 7
    instrument_functions[XED_ICLASS_AND] = &Instrument_AND;	// 8

    //  instrument_functions[XED_ICLASS_DAA] = &Instrument_DAA; // 11
    instrument_functions[XED_ICLASS_SUB] = &Instrument_SUB;	// 12

    //  instrument_functions[XED_ICLASS_DAS] = &Instrument_DAS; // 14
    instrument_functions[XED_ICLASS_XOR] = &Instrument_XOR;	// 15

    //  instrument_functions[XED_ICLASS_AAA] = &Instrument_AAA; // 17
    instrument_functions[XED_ICLASS_CMP] = &Instrument_CMP;	// 18

    //  instrument_functions[XED_ICLASS_AAS] = &Instrument_AAS; // 20
    instrument_functions[XED_ICLASS_INC] = &Instrument_INC;	// 21
    instrument_functions[XED_ICLASS_DEC] = &Instrument_DEC;	// 22

    //  instrument_functions[XED_ICLASS_PUSHAD] = &Instrument_PUSHAD; // 25
    //  instrument_functions[XED_ICLASS_POPAD] = &Instrument_POPAD; // 27
    //  instrument_functions[XED_ICLASS_BOUND] = &Instrument_BOUND; // 28
    //  instrument_functions[XED_ICLASS_ARPL] = &Instrument_ARPL; // 29

    instrument_functions[XED_ICLASS_IMUL] = &Instrument_IMUL;	// 35
    //  instrument_functions[XED_ICLASS_INSB] = &Instrument_INSB; // 36

    //  instrument_functions[XED_ICLASS_INSD] = &Instrument_INSD; // 38
    //  instrument_functions[XED_ICLASS_OUTSB] = &Instrument_OUTSB; // 39

    //  instrument_functions[XED_ICLASS_OUTSD] = &Instrument_OUTSD; // 41
    instrument_functions[XED_ICLASS_JO] = &Instrument_Jcc;	//42
    instrument_functions[XED_ICLASS_JNO] = &Instrument_Jcc;	//43
    instrument_functions[XED_ICLASS_JB] = &Instrument_Jcc;	//43
    instrument_functions[XED_ICLASS_JNB] = &Instrument_Jcc;	//45
    instrument_functions[XED_ICLASS_JZ] = &Instrument_Jcc;	//46
    instrument_functions[XED_ICLASS_JNZ] = &Instrument_Jcc;	//47
    instrument_functions[XED_ICLASS_JBE] = &Instrument_Jcc;	//48
    instrument_functions[XED_ICLASS_JNBE] = &Instrument_Jcc;	//49
    instrument_functions[XED_ICLASS_JS] = &Instrument_Jcc;	//50
    instrument_functions[XED_ICLASS_JNS] = &Instrument_Jcc;	//51
    instrument_functions[XED_ICLASS_JP] = &Instrument_Jcc;	//52
    instrument_functions[XED_ICLASS_JNP] = &Instrument_Jcc;	//53
    instrument_functions[XED_ICLASS_JL] = &Instrument_Jcc;	//54
    instrument_functions[XED_ICLASS_JNL] = &Instrument_Jcc;	//55
    instrument_functions[XED_ICLASS_JLE] = &Instrument_Jcc;	//56
    instrument_functions[XED_ICLASS_JNLE] = &Instrument_Jcc;	//57

    instrument_functions[XED_ICLASS_TEST] = &Instrument_TEST;	//59
    instrument_functions[XED_ICLASS_XCHG] = &Instrument_XCHG;	//60
    instrument_functions[XED_ICLASS_MOV] = &Instrument_MOV;	//61
    instrument_functions[XED_ICLASS_LEA] = &Instrument_LEA;	//62

    instrument_functions[XED_ICLASS_PAUSE] = &Instrument_PAUSE;	//64

    instrument_functions[XED_ICLASS_CWDE] = &Instrument_CWDE;	//67

    instrument_functions[XED_ICLASS_CDQ] = &Instrument_CDQ;	//70
    instrument_functions[XED_ICLASS_CALL_FAR] = &Instrument_CALL_FAR;	//71
    //  instrument_functions[XED_ICLASS_WAIT] = &Instrument_WAIT; //72

    instrument_functions[XED_ICLASS_PUSHFD] = &Instrument_PUSHFD;	//74

    instrument_functions[XED_ICLASS_POPFD] = &Instrument_POPFD;	//77

    //  instrument_functions[XED_ICLASS_POPFD] = &Instrument_SAHF; //79
    //  instrument_functions[XED_ICLASS_POPFD] = &Instrument_LAHF; //80
    instrument_functions[XED_ICLASS_MOVSB] = &Instrument_MOVSB;	//81
    instrument_functions[XED_ICLASS_MOVSW] = &Instrument_MOVSW;	//82
    instrument_functions[XED_ICLASS_MOVSD] = &Instrument_MOVSD;	//83

    instrument_functions[XED_ICLASS_CMPSB] = &Instrument_CMPSB;	//85

    //  instrument_functions[XED_ICLASS_CMPSD] = &Instrument_CMPSD; //87

    instrument_functions[XED_ICLASS_STOSB] = &Instrument_STOSB;	//89
    instrument_functions[XED_ICLASS_STOSW] = &Instrument_STOSW; //90
    instrument_functions[XED_ICLASS_STOSD] = &Instrument_STOSD;	//91

    //  instrument_functions[XED_ICLASS_LODSB] = &Instrument_LODSB; //93

    //  instrument_functions[XED_ICLASS_LODSD] = &Instrument_LODSD; //95

    instrument_functions[XED_ICLASS_SCASB] = &Instrument_SCASB;	//97

    //  instrument_functions[XED_ICLASS_SCASD] = &Instrument_SCASD; //99

    instrument_functions[XED_ICLASS_RET_NEAR] = &Instrument_RET_NEAR;	//102
    //  instrument_functions[XED_ICLASS_LES] = &Instrument_LES; //103
    //  instrument_functions[XED_ICLASS_LDS] = &Instrument_LDS; //104

    //  instrument_functions[XED_ICLASS_ENTER] = &Instrument_ENTER; //106
    instrument_functions[XED_ICLASS_LEAVE] = &Instrument_LEAVE;	//107
    instrument_functions[XED_ICLASS_RET_FAR] = &Instrument_RET_FAR; //108
    //  instrument_functions[XED_ICLASS_INT3] = &Instrument_INT3; //109
    instrument_functions[XED_ICLASS_INT] = &Instrument_INT;	//110
    //  instrument_functions[XED_ICLASS_INT0] = &Instrument_INT0; //111

    //  instrument_functions[XED_ICLASS_IRETD] = &Instrument_IRETD; //113

    //  instrument_functions[XED_ICLASS_AAM] = &Instrument_AAM; //115
    //  instrument_functions[XED_ICLASS_AAD] = &Instrument_AAD; //116
    //  instrument_functions[XED_ICLASS_SALC] = &Instrument_SALC; //117
    //  instrument_functions[XED_ICLASS_XLAT] = &Instrument_XLAT; //118

    //  instrument_functions[XED_ICLASS_LOOPNE] = &Instrument_LOOPNE; //120
    //  instrument_functions[XED_ICLASS_LOOPE] = &Instrument_LOOPE; //121
    //  instrument_functions[XED_ICLASS_LOOP] = &Instrument_LOOP; //122
    instrument_functions[XED_ICLASS_JRCXZ] = &Instrument_Jcc;	//123
    //  instrument_functions[XED_ICLASS_IN] = &Instrument_IN; //124
    //  instrument_functions[XED_ICLASS_OUT] = &Instrument_OUT; //125
    instrument_functions[XED_ICLASS_CALL_NEAR] = &Instrument_CALL_NEAR;	//126
    instrument_functions[XED_ICLASS_JMP] = &Instrument_JMP;	//127
    //  instrument_functions[XED_ICLASS_JMP_FAR] = &Instrument_JMP_FAR; //128

    //  instrument_functions[XED_ICLASS_INT_l] = &Instrument_INT_l; //130

    instrument_functions[XED_ICLASS_HLT] = &Instrument_HLT;	//133
    //  instrument_functions[XED_ICLASS_CMC] = &Instrument_CMC; //134

    //  instrument_functions[XED_ICLASS_CLC] = &Instrument_CLC; //136
    //  instrument_functions[XED_ICLASS_STC] = &Instrument_STC; //137
    //  instrument_functions[XED_ICLASS_CLI] = &Instrument_CLI; //138
    //  instrument_functions[XED_ICLASS_STI] = &Instrument_STI; //139
    instrument_functions[XED_ICLASS_CLD] = &Instrument_CLD;	//140
    instrument_functions[XED_ICLASS_STD] = &Instrument_STD;	//141

    instrument_functions[XED_ICLASS_RDTSC] = &Instrument_RDTSC;	//169

    instrument_functions[XED_ICLASS_CMOVB] = &Instrument_CMOVcc;	//177
    instrument_functions[XED_ICLASS_CMOVNB] = &Instrument_CMOVcc;	//178
    instrument_functions[XED_ICLASS_CMOVZ] = &Instrument_CMOVcc;	//179
    instrument_functions[XED_ICLASS_CMOVNZ] = &Instrument_CMOVcc;	//180
    instrument_functions[XED_ICLASS_CMOVBE] = &Instrument_CMOVcc;	//181
    instrument_functions[XED_ICLASS_CMOVNBE] = &Instrument_CMOVcc;	//182

    //  instrument_functions[XED_ICLASS_EMMS] = &Instrument_EMMS; //216

    instrument_functions[XED_ICLASS_SETB] = &Instrument_SETcc;	//222
    instrument_functions[XED_ICLASS_SETNB] = &Instrument_SETcc;	//223
    instrument_functions[XED_ICLASS_SETZ] = &Instrument_SETcc;	//224
    instrument_functions[XED_ICLASS_SETNZ] = &Instrument_SETcc;	//225
    instrument_functions[XED_ICLASS_SETBE] = &Instrument_SETcc;	//226
    instrument_functions[XED_ICLASS_SETNBE] = &Instrument_SETcc;	//227
    //  instrument_functions[XED_ICLASS_CPUID] = &Instrument_CPUID; //228
    //  instrument_functions[XED_ICLASS_BT] = &Instrument_BT; //229
    instrument_functions[XED_ICLASS_SHLD] = &Instrument_SHLD;	//230
    instrument_functions[XED_ICLASS_CMPXCHG] = &Instrument_CMPXCHG;	//231

    //  instrument_functions[XED_ICLASS_BTR] = &Instrument_BTR; //233

    instrument_functions[XED_ICLASS_MOVZX] = &Instrument_MOVZX;	//236
    instrument_functions[XED_ICLASS_XADD] = &Instrument_XADD;	//237

    //  instrument_functions[XED_ICLASS_PSRLQ] = &Instrument_PSRLQ; //250
    //  instrument_functions[XED_ICLASS_PADDQ] = &Instrument_PADDQ; //251

    //  instrument_functions[XED_ICLASS_MOVQ] = &Instrument_MOVQ; //255

    //  instrument_functions[XED_ICLASS_MOVQ2Q] = &Instrument_MOVDQ2Q; //258

    //  instrument_functions[XED_ICLASS_PSLLQ] = &Instrument_PSLLQ; //272
    //  instrument_functions[XED_ICLASS_PMULUDQ] = &Instrument_PMULUDQ; //273

    //  instrument_functions[XED_ICLASS_UD2] = &Instrument_UD2; //281

    instrument_functions[XED_ICLASS_CMOVS] = &Instrument_CMOVcc;	//307
    instrument_functions[XED_ICLASS_CMOVNS] = &Instrument_CMOVcc;	//308

    instrument_functions[XED_ICLASS_CMOVL] = &Instrument_CMOVcc;	//311
    instrument_functions[XED_ICLASS_CMOVNL] = &Instrument_CMOVcc;	//312
    instrument_functions[XED_ICLASS_CMOVLE] = &Instrument_CMOVcc;	//313
    instrument_functions[XED_ICLASS_CMOVNLE] = &Instrument_CMOVcc;	//314

    //  instrument_functions[XED_ICLASS_MOVD] = &Instrument_MOVD; //350
    //  instrument_functions[XED_ICLASS_MOVDQU] = &Instrument_MOVDQU; //351

    //  instrument_functions[XED_ICLASS_MOVDQA] = &Instrument_MOVDQA; //354

    instrument_functions[XED_ICLASS_SETS] = &Instrument_SETcc;	//361

    instrument_functions[XED_ICLASS_SETL] = &Instrument_SETcc;	//365
    instrument_functions[XED_ICLASS_SETNL] = &Instrument_SETcc;	//366
    instrument_functions[XED_ICLASS_SETLE] = &Instrument_SETcc;	//367
    instrument_functions[XED_ICLASS_SETNLE] = &Instrument_SETcc;	//368

    //  instrument_functions[XED_ICLASS_BTS] = &Instrument_BTS; //370
    instrument_functions[XED_ICLASS_SHRD] = &Instrument_SHRD;	//371

    //  instrument_functions[XED_ICLASS_BSF] = &Instrument_BSF; //376
    //  instrument_functions[XED_ICLASS_BSR] = &Instrument_BSR; //377
    instrument_functions[XED_ICLASS_MOVSX] = &Instrument_MOVSX;	//378
    instrument_functions[XED_ICLASS_BSWAP] = &Instrument_BSWAP;	//379

    //  instrument_functions[XED_ICLASS_PAND] = &Instrument_PAND; //383

    //  instrument_functions[XED_ICLASS_PSUBSW] = &Instrument_PSUBSW; //389

    //  instrument_functions[XED_ICLASS_POR] = &Instrument_POR; //391

    //  instrument_functions[XED_ICLASS_PXOR] = &Instrument_PXOR; //395

    //  instrument_functions[XED_ICLASS_ROL] = &Instrument_ROL; //472
    //  instrument_functions[XED_ICLASS_ROR] = &Instrument_ROR; //473
    //  instrument_functions[XED_ICLASS_RCL] = &Instrument_RCL; //474
    //  instrument_functions[XED_ICLASS_RCR] = &Instrument_RCR; //475
    instrument_functions[XED_ICLASS_SHL] = &Instrument_SHL;	//476
    instrument_functions[XED_ICLASS_SHR] = &Instrument_SHR;	//477
    instrument_functions[XED_ICLASS_SAR] = &Instrument_SAR;	//478
    instrument_functions[XED_ICLASS_NOT] = &Instrument_NOT;	//479
    instrument_functions[XED_ICLASS_NEG] = &Instrument_NEG;	//480
    instrument_functions[XED_ICLASS_MUL] = &Instrument_MUL;	//481
    instrument_functions[XED_ICLASS_DIV] = &Instrument_DIV;	//482
    instrument_functions[XED_ICLASS_IDIV] = &Instrument_IDIV;	//483

    instrument_functions[XED_ICLASS_LDMXCSR] = &Instrument_LDMXCSR;	//507
    instrument_functions[XED_ICLASS_STMXCSR] = &Instrument_STMXCSR;	//508

    instrument_functions[XED_ICLASS_FLDCW] = &Instrument_FLDCW;	//527

    instrument_functions[XED_ICLASS_FNSTCW] = &Instrument_FNSTCW;	//529
}
