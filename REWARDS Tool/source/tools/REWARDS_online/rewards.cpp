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

/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2009 Intel Corporation. All rights reserved.

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

/* ===================================================================== */
/*
*/

/* ===================================================================== */
/*! @file
 *  This file contains a tool that generates instructions traces with values.
 *  It is designed to help debugging.
 */

#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "inttypes.h"
#include "rewards.h"
#include "tls.h"
#include "tag.h"

using namespace INSTLIB;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */
KNOB < THREADID > KnobWatchThread
    (KNOB_MODE_WRITEONCE, "pintool", "watch_thread", "-1", "thread to watch, -1 for all");
KNOB < BOOL > KnobFlush
    (KNOB_MODE_WRITEONCE, "pintool", "flush", "0", "Flush output after every instruction");
KNOB < BOOL > KnobSymbols
    (KNOB_MODE_WRITEONCE, "pintool", "symbols", "1", "Include symbol information");
KNOB < BOOL > KnobLines
    (KNOB_MODE_WRITEONCE, "pintool", "lines", "1", "Include line number information");
KNOB < BOOL > KnobTraceInstructions
    (KNOB_MODE_WRITEONCE, "pintool", "instruction", "0", "Trace instructions");
KNOB < BOOL > KnobTraceCalls
    (KNOB_MODE_WRITEONCE, "pintool", "call", "0", "Trace calls");
KNOB < BOOL > KnobTraceMemory
    (KNOB_MODE_WRITEONCE, "pintool", "memory", "0", "Trace memory");
KNOB < BOOL > KnobSilent
    (KNOB_MODE_WRITEONCE, "pintool", "silent", "0", "Do everything but write file (for debugging).");
KNOB < BOOL > KnobEarlyOut
    (KNOB_MODE_WRITEONCE, "pintool", "early_out", "0", "Exit after tracing the first region.");
KNOB < BOOL > KnobDebug
    (KNOB_MODE_WRITEONCE, "pintool", "debug", "0", "Debug output");
KNOB < BOOL > KnobTrace
    (KNOB_MODE_WRITEONCE, "pintool", "trace", "0", "Trace output");
KNOB < string > KnobStaticlinkFunctions
    (KNOB_MODE_WRITEONCE, "pintool", "staticlink_functions", "", "Statically linked function information");
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This pin tool collects an instruction trace for debugging\n\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

PIN_LOCK pin_lock;

LOCALVAR INT32 enabled = 0;

LOCALVAR FILTER filter;

LOCALVAR ICOUNT icount;

LOCALFUN inline BOOL Emit(THREADID threadid)
{
    if (!enabled ||
            KnobSilent ||
            (KnobWatchThread != static_cast < THREADID > (-1)
             && KnobWatchThread != threadid))
        return false;
    return true;
}

LOCALFUN inline VOID Flush()
{
    if (KnobFlush)
        fflush(GetTLS()->fptrace);
}

/* ===================================================================== */

LOCALFUN VOID Fini(int, VOID * v);

LOCALFUN VOID Handler(CONTROL_EVENT ev, VOID*, CONTEXT* ctxt, VOID*, THREADID)
{
    switch (ev) {
    case CONTROL_START:
        enabled = 1;
        PIN_RemoveInstrumentation();
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        // So that the rest of the current trace is re-instrumented.
        if (ctxt)
            PIN_ExecuteAt(ctxt);
#endif
        break;
    case CONTROL_STOP:
        enabled = 0;
        PIN_RemoveInstrumentation();
        if (KnobEarlyOut) {
            cerr << "Exiting due to -early_out" << endl;
            Fini(0, NULL);
            exit(0);
        }
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        // So that the rest of the current trace is re-instrumented.
        if (ctxt)
            PIN_ExecuteAt(ctxt);
#endif
        break;
    default:
        ASSERTX(false);
    }
}

/* ===================================================================== */

LOCALFUN VOID EmitNoValues(THREADID threadid, string * str)
{
    if (!Emit(threadid))
        return;

    TPRINT ("%s\n", str->c_str());
    Flush();
}

LOCALFUN VOID Emit1Values(THREADID threadid, string * str,
            string * reg1str, ADDRINT reg1val)
{
    if (!Emit(threadid))
        return;

    TPRINT ("%s | %s = %x\n", str->c_str(), reg1str->c_str(), reg1val);
    Flush();
}

LOCALFUN VOID Emit2Values(THREADID threadid, string * str,
            string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val)
{
    if (!Emit(threadid))
        return;

    TPRINT ("%s | %s = %x , %s =%x\n", str->c_str(),
           reg1str->c_str(), reg1val, reg2str->c_str(), reg2val);
    Flush();
}

LOCALFUN VOID Emit3Values(THREADID threadid, string * str,
            string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val,
            string * reg3str, ADDRINT reg3val)
{
    if (!Emit(threadid))
        return;

    TPRINT ("%s | %s = %x , %s =%x , %s = %x\n", str->c_str(),
           reg1str->c_str(), reg1val, reg2str->c_str(), reg2val,
           reg3str->c_str(), reg3val);
    Flush();
}

VOID Emit4Values(THREADID threadid, string * str,
            string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val,
            string * reg3str, ADDRINT reg3val, string * reg4str, ADDRINT reg4val)
{
    if (!Emit(threadid))
        return;

    TPRINT ("%s | %s = %x , %s =%x , %s = %x, %s = %x\n", str->c_str(),
           reg1str->c_str(), reg1val, reg2str->c_str(), reg2val,
           reg3str->c_str(), reg3val, reg4str->c_str(), reg4val);
    Flush();
}

const UINT32 MaxEmitArgs = 4;

AFUNPTR emitFuns[] = {
    AFUNPTR(EmitNoValues), AFUNPTR(Emit1Values), AFUNPTR(Emit2Values),
    AFUNPTR(Emit3Values), AFUNPTR(Emit4Values)
};

/* ===================================================================== */
#if !defined(TARGET_IPF)
VOID EmitXMM(THREADID threadid, UINT32 regno, PIN_REGISTER * xmm)
{
    if (!Emit(threadid))
        return;
}

VOID AddXMMEmit(INS ins, IPOINT point, REG xmm_dst)
{
    INS_InsertCall(ins, point, AFUNPTR(EmitXMM), IARG_THREAD_ID,
                   IARG_UINT32, xmm_dst - REG_XMM0,
                   IARG_REG_CONST_REFERENCE, xmm_dst, IARG_END);
}
#endif

VOID AddEmit(INS ins, IPOINT point, string & traceString,
             UINT32 regCount, REG regs[])
{
    if (regCount > MaxEmitArgs)
        regCount = MaxEmitArgs;

    IARGLIST args = IARGLIST_Alloc();
    for (UINT32 i = 0; i < regCount; i++) {
        IARGLIST_AddArguments(args, IARG_PTR,
                              new string(REG_StringShort(regs[i])),
                              IARG_REG_VALUE, regs[i], IARG_END);
    }

    INS_InsertCall(ins, point, emitFuns[regCount], IARG_THREAD_ID,
                   IARG_PTR, new string(traceString),
                   IARG_IARGLIST, args, IARG_END);
    IARGLIST_Free(args);
}


LOCALVAR VOID *WriteEa[PIN_MAX_THREADS];

LOCALINLINE VOID CaptureWriteEa(THREADID threadid, VOID * addr)
{
    WriteEa[threadid] = addr;
}

LOCALINLINE VOID ShowN(UINT32 n, VOID * ea)
{
}


VOID EmitWrite(THREADID threadid, UINT32 size)
{
    if (!Emit(threadid))
        return;

    TPRINT ("                               Write ");

    VOID *ea = WriteEa[threadid];

    switch (size) {
    case 0:
        TPRINT ("0 repeat count\n");
        break;

    case 1:
    {
        UINT8 x;
        PIN_SafeCopy(&x, static_cast < UINT8 * >(ea), 1);
        TPRINT ("%x 08 %08x\n", ea, x);
    }
    break;

    case 2:
    {
        UINT16 x;
        PIN_SafeCopy(&x, static_cast < UINT16 * >(ea), 2);
        TPRINT ("%x 16 %08x\n", ea, x);
    }
    break;

    case 4:
    {
        UINT32 x;
        PIN_SafeCopy(&x, static_cast < UINT32 * >(ea), 4);
        TPRINT ("%x 32 %08x\n", ea, x);
    }
    break;

    case 8:
    {
        UINT64 x;
        PIN_SafeCopy(&x, static_cast < UINT64 * >(ea), 8);
        TPRINT ("%x 64 %08x\n", ea, x);
    }
    break;

    default:
        TPRINT ("%x %d default\n", ea, size * 8);
        ShowN(size, ea);
        break;
    }
    Flush();
}

VOID EmitRead(THREADID threadid, VOID * ea, UINT32 size)
{
    if (!Emit(threadid))
        return;

    TPRINT ("                               Read ");

    switch (size) {
    case 0:
        TPRINT ("0 repeat count\n");
        break;

    case 1:
    {
        UINT8 x;
        PIN_SafeCopy(&x, static_cast < UINT8 * >(ea), 1);
        TPRINT ("%x 08 %08x\n", ea, x);
    }
    break;

    case 2:
    {
        UINT16 x;
        PIN_SafeCopy(&x, static_cast < UINT16 * >(ea), 2);
        TPRINT ("%x 16 %08x\n", ea, x);
    }
    break;

    case 4:
    {
        UINT32 x;
        PIN_SafeCopy(&x, static_cast < UINT32 * >(ea), 4);
        TPRINT ("%x 32 %08x\n", ea, x);
    }
    break;

    case 8:
    {
        UINT64 x;
        PIN_SafeCopy(&x, static_cast < UINT64 * >(ea), 8);
        TPRINT ("%x 64 %08x\n", ea, x);
    }
    break;

    default:
        ShowN(size, ea);
        TPRINT ("%x %d default\n", ea, size * 8);
        break;
    }
    Flush();
}

LOCALVAR INT32 indent = 0;

VOID Indent()
{
    for (INT32 i = 0; i < indent; i++)
        TPRINT ("| ");
}

VOID EmitICount()
{
    TPRINT ("total instruction %x\n", icount.Count());
}

VOID EmitDirectCall(THREADID threadid, string * str, INT32 tailCall,
                    ADDRINT arg0, ADDRINT arg1)
{
    if (!Emit(threadid))
        return;

    // A tail call is like an implicit return followed by an immediate call
    if (tailCall)
        indent--;

    //EmitICount();
    Indent();
    TPRINT ("%s ( %x , %x , ...)\n", str->c_str(), arg0, arg1);
    Flush();
    indent++;
}

string FormatAddress(ADDRINT address, RTN rtn)
{
    string s = StringFromAddrint(address);

    if (KnobSymbols && RTN_Valid(rtn)) {
        s += " " + IMG_Name(SEC_Img(RTN_Sec(rtn))) + ":";
        s += RTN_Name(rtn);

        ADDRINT delta = address - RTN_Address(rtn);
        if (delta != 0) {
            s += "+" + hexstr(delta, 4);
        }
    }

    if (KnobLines) {
        INT32 line;
        string file;

        PIN_GetSourceLocation(address, NULL, &line, &file);

        if (file != "") {
            s += " (" + file + ":" + decstr(line) + ")";
        }
    }
    return s;
}

VOID EmitIndirectCall(THREADID threadid, string * str, ADDRINT target,
                      ADDRINT arg0, ADDRINT arg1)
{
    if (!Emit(threadid))
        return;

    PIN_LockClient();
    string s = FormatAddress(target, RTN_FindByAddress(target));
    PIN_UnlockClient();

    //EmitICount();
    Indent();
    TPRINT ("%s", str->c_str());
    TPRINT ("%s ( %x , %x , ...)\n", s.c_str(), arg0, arg1);
    Flush();

    indent++;
}

VOID EmitReturn(THREADID threadid, string * str, ADDRINT ret0)
{
    if (!Emit(threadid))
        return;

    //EmitICount();
    indent--;
    if (indent < 0) {
        TPRINT ("@@@ return underflow\n");
        indent = 0;
    }

    Indent();
    TPRINT ("%s returns: %x\n", str->c_str(), ret0);
    Flush();
}

extern uint32_t time_stamp_g;

string GetRtnName(ADDRINT pc)
{
    string s;
    PIN_LockClient();
    RTN rtn = RTN_FindByAddress(pc);
    PIN_UnlockClient();

    if(RTN_Valid(rtn))
    {
        s =  RTN_Name(rtn);
        ADDRINT delta = pc - RTN_Address(rtn);
        if(delta !=0)
        {
            s+="+" + hexstr(delta,4);
        }
    }
    else
    {
        s= StringFromAddrint(pc);
    }
    return s;
}

VOID UpdateTimeStamp()
{
    // increase time_stamp whenever a function is called.
    GetLock(&pin_lock, PIN_ThreadId() + 1);
    time_stamp_g ++;
    ReleaseLock(&pin_lock);
}

VOID CallTrace(TRACE trace, INS ins)
{
    if (!KnobTraceCalls)
        return;

    if (INS_IsCall(ins) && !INS_IsDirectBranchOrCall(ins)) {

        // Indirect call
        string s = "Call ";
        s += FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
        s += " -> ";

        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitIndirectCall),
                IARG_THREAD_ID,
                IARG_PTR, new string(s),
                IARG_BRANCH_TARGET_ADDR,
                IARG_G_ARG0_CALLER, IARG_G_ARG1_CALLER, IARG_END);

    } else if (INS_IsDirectBranchOrCall(ins)) {

        // Is this a tail call?
        RTN sourceRtn = TRACE_Rtn(trace);
        RTN destRtn =
            RTN_FindByAddress(INS_DirectBranchOrCallTargetAddress(ins));

        if (INS_IsCall(ins)	// conventional call
                || sourceRtn != destRtn	// tail call
           ) {
            string s = "";
            if (!INS_IsCall(ins))
                s += "Tailcall ";
            else {
                if (INS_IsProcedureCall(ins))
                    s += "Call ";
                else
                    s += "PcMaterialization ";
            }

            //s += INS_Mnemonic(ins) + " ";
            BOOL tailcall = !(INS_IsCall(ins) && INS_IsProcedureCall(ins));

            s += FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
            s += " -> ";

            ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
            s += FormatAddress(target, RTN_FindByAddress(target));

            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitDirectCall),
                    IARG_THREAD_ID,
                    IARG_PTR, new string(s),
                    IARG_UINT32, tailcall,
                    IARG_G_ARG0_CALLER, IARG_G_ARG1_CALLER, IARG_END);
        }
    } else if (INS_IsRet(ins)) {

        RTN rtn = TRACE_Rtn(trace);

#if defined(TARGET_LINUX) && defined(TARGET_IA32)
//        if( RTN_Name(rtn) ==  "_dl_debug_state") return;
        if (RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_runtime_resolve")
            return;
#endif

        string tracestring =
            "Return " + FormatAddress(INS_Address(ins), rtn);
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitReturn),
                IARG_THREAD_ID,
                IARG_PTR, new string(tracestring),
                IARG_G_RESULT0, IARG_END);
    }
}


VOID InstructionTrace(TRACE trace, INS ins)
{
    if (!KnobTraceInstructions)
        return;

    ASSERTX(INS_Address(ins));

    // Format the string at instrumentation time
    string traceString = "";
    string astring = FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
    for (INT32 length = astring.length(); length < 30; length++) {
        traceString += " ";
    }
    traceString = astring + traceString;

    traceString += " " + INS_Disassemble(ins);

    for (INT32 length = traceString.length(); length < 80; length++) {
        traceString += " ";
    }

    INT32 regCount = 0;
    REG regs[20];
#if !defined(TARGET_IPF)
    REG xmm_dst = REG_INVALID();
#endif

    for (UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++) {
        REG x = REG_FullRegName(INS_RegW(ins, i));

        if (REG_is_gr(x)
#if defined(TARGET_IA32)
                || x == REG_EFLAGS
#elif defined(TARGET_IA32E)
                || x == REG_RFLAGS
#elif defined(TARGET_IPF)
                || REG_is_pr(x)
                || REG_is_br(x)
#endif
           ) {
            regs[regCount] = x;
            regCount++;
        }
#if !defined(TARGET_IPF)
        if (REG_is_xmm(x))
            xmm_dst = x;
#endif

    }

#if defined(TARGET_IPF)
    if (INS_IsCall(ins) || INS_IsRet(ins)
            || INS_Category(ins) == CATEGORY_ALLOC) {
        regs[regCount] = REG_CFM;
        regCount++;
    }
#endif

    if (INS_HasFallThrough(ins)) {
        AddEmit(ins, IPOINT_AFTER, traceString, regCount, regs);
    }
    if (INS_IsBranchOrCall(ins)) {
        AddEmit(ins, IPOINT_TAKEN_BRANCH, traceString, regCount, regs);
    }
#if !defined(TARGET_IPF)
    if (xmm_dst != REG_INVALID()) {
        if (INS_HasFallThrough(ins))
            AddXMMEmit(ins, IPOINT_AFTER, xmm_dst);
        if (INS_IsBranchOrCall(ins))
            AddXMMEmit(ins, IPOINT_TAKEN_BRANCH, xmm_dst);
    }
#endif
}

VOID MemoryTrace(INS ins)
{
    if (!KnobTraceMemory)
        return;

    if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CaptureWriteEa),
                       IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_END);

        if (INS_HasFallThrough(ins)) {
            INS_InsertPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(EmitWrite),
                                     IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
        if (INS_IsBranchOrCall(ins)) {
            INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(EmitWrite),
                                     IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
    }

    if (INS_HasMemoryRead2(ins)) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(EmitRead),
                                 IARG_THREAD_ID, IARG_MEMORYREAD2_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_END);
    }

    if (INS_IsMemoryRead(ins) && !INS_IsPrefetch(ins)) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(EmitRead),
                                 IARG_THREAD_ID, IARG_MEMORYREAD_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_END);
    }
}

/* ===================================================================== */

VOID Trace(TRACE trace, VOID * v)
{
    Instrument_Trace(trace);

    if (!filter.SelectTrace(trace))
        return;

    if (enabled) {
        for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
            for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
                InstructionTrace(trace, ins);
                CallTrace(trace, ins);
                MemoryTrace(ins);
            }
        }
    }
}

/* ===================================================================== */

VOID Fini(int, VOID * v)
{
    fclose(fptrace);
    fclose(fperr);
}

SpaceList *pStack = NULL;

VOID ThreadStart(THREADID tid, CONTEXT * ctxt, INT32 flags, VOID * v)
{
    NewTLS(tid);
    DEBUG ("thread %d start OSid %d \n", tid, PIN_ThreadId());

    ADDRINT fs = 0, stacktop = 0, stackbot = 0;
    fs = PIN_GetContextReg(ctxt, REG_SEG_FS_BASE);
    PIN_SafeCopy(&stacktop, (ADDRINT*)(fs + 0x04), 4);
    PIN_SafeCopy(&stackbot, (ADDRINT*)(fs + 0x08), 4);

    if (stacktop != 0 && stackbot != 0) {
        InsertToSpaceList(&pStack, stackbot, stacktop);

        char temp_filename[128];
        sprintf(temp_filename, "%s/global_space.out", filename_prefix);
        FILE *fp = fopen(temp_filename, "a");
        fprintf (fp,"%08x %08x [thread %d stack]\n", stackbot, stacktop, tid);
        fclose(fp);
    }
}

VOID ThreadFini(THREADID tid, const CONTEXT * ctxt, INT32 code, VOID * v)
{
    DEBUG ("thread %d end %d \n", tid, PIN_ThreadId());
}

bool IsInStackSpace(ADDRINT addr)
{
    return IsInSpaceList(&pStack, addr);
}

/* ===================================================================== */

static VOID OnSig(THREADID tid,
                  CONTEXT_CHANGE_REASON reason,
                  const CONTEXT * ctxtFrom,
                  CONTEXT * ctxtTo, INT32 sig, VOID * v)
{
    DEBUG ("OnSig\n");

    if (ctxtFrom != 0) {
        ADDRINT address = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
        DEBUG ("SIG Signal %d on thread %d at address %x\n",
                     sig, tid, address);
    }

    switch (reason) {
    case CONTEXT_CHANGE_REASON_FATALSIGNAL:
        DEBUG ("FATASIG %d\n", sig);
        break;
    case CONTEXT_CHANGE_REASON_SIGNAL:
        DEBUG ("SIG %d\n", sig);
        break;
    case CONTEXT_CHANGE_REASON_SIGRETURN:
        DEBUG ("SIGRET\n");
        break;
    case CONTEXT_CHANGE_REASON_APC:
        DEBUG ("APC\n");
        break;
    case CONTEXT_CHANGE_REASON_EXCEPTION:
        DEBUG ("EXCEPTION\n");
        break;
    case CONTEXT_CHANGE_REASON_CALLBACK:
        DEBUG ("CALLBACK\n");
        break;
    default:
        break;
    }
}

/* ===================================================================== */

LOCALVAR CONTROL control;

/* ===================================================================== */

//FILE *semantic_file;

FILE *syscall_file;
FILE *fptrace;
FILE *fperr;

//int msglevel;			/* the higher, the more messages... */
char filename_prefix[32];
char ImageName[128];

PIN_LOCK fptrace_lock;

int main(int argc, CHAR * argv[])
{
    unsigned int PID;
    char temp_file_name[256];
    char map_fname[20];

    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    PIN_InitSymbols();

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--")) {
            strcpy(ImageName, ParseImageName(argv[i + 1]));
            break;
        }
    }

    LoadStaticlinkFunction(KnobStaticlinkFunctions);

    InitTLS();

//    msglevel = (KnobDebug ? 0 : 9);

    InitLock(&pin_lock);
    InitLock(&fptrace_lock);

    PID = PIN_GetTid();
    sprintf(filename_prefix, "%s-%d", ImageName, PID);

    sprintf(temp_file_name, "mkdir %s", filename_prefix);
    system(temp_file_name);

//    sprintf(temp_file_name, "%s/semantic.out", filename_prefix);
//    semantic_file = fopen(temp_file_name, "w");

    sprintf(temp_file_name, "%s/syscall.out", filename_prefix);
    syscall_file = fopen(temp_file_name, "w");

    sprintf(temp_file_name, "%s/alltrace.out", filename_prefix);
    fptrace = fopen(temp_file_name, "w");

    sprintf(temp_file_name, "%s/err.out", filename_prefix);
    fperr = fopen(temp_file_name, "w");

    control.CheckKnobs(Handler, 0);

    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddContextChangeFunction(OnSig, 0);

    IMG_AddInstrumentFunction(ImageLoad, 0);

    setup_hook();
    INS_AddInstrumentFunction(InstrumentINS, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    PIN_AddSyscallEntryFunction(SyscallEntry, 0);
    PIN_AddSyscallExitFunction(SyscallExit, 0);

    filter.Activate();
    //icount.Activate();

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
