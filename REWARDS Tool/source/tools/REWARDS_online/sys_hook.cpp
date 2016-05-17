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

//#include <sys/socket.h>
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
/*
 *  This file contains an ISA-portable PIN tool for tracing system calls
 */

#include <stdio.h>

/*
#if defined(TARGET_MAC)
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
*/

#include "rewards.h"
#include "syscall.h"
#include "tls.h"
#include "tag.h"

static unsigned int GetSyscallNum()
{
    return GetTLS()->syscall_number;
}
static void SetSyscallNum(unsigned int syscall_no)
{
    GetTLS()->syscall_number = syscall_no;
}

extern uint32_t time_stamp_g;
extern PIN_LOCK pin_lock;

static VOID SetSyscallArgTagType(const CONTEXT *ctxt, SYSCALL_STANDARD std, int i, T_TYPE t)
{
    if (std != SYSCALL_STANDARD_IA32_WINDOWS_FAST) {
        DEBUG ("SYSCALL with NO SYSCALL_STANDARD_IA32_WINDOWS_FAST\n");
        return;
    }

    ADDRINT pc = PIN_GetContextReg(ctxt, REG_INST_PTR);
    ADDRINT edx = PIN_GetContextReg(ctxt, REG_EDX);
    ADDRINT ebp = PIN_GetContextReg(ctxt, REG_EBP);
    ADDRINT addr = edx + 4 * (i+2);

    ResolveTAG(GetMTAG(addr, 4, pc), t);
    
    ADDRINT arg = 0;
    PIN_SafeCopy(&arg, (ADDRINT*)addr, 4); 
    
    DEBUG ("syscall arg %d: 0x%08x 0x%08x\n", i, addr, arg);

    if (t & T_OUT) {
        DEBUG ("  OUT parameter, 0x%08x: ", arg);

        if (arg == 0) {
            DEBUG ("<NULL>\n");
        }
        else {
            ADDRINT val;
            size_t read = PIN_SafeCopy(&val, (ADDRINT*)arg, 4);

            if (read != 4) {
                DEBUG ("OUT parameter seems not valid pointer in syscall 0x%08x arg %d, 0x%08x\n",
                            PIN_GetSyscallNumber(ctxt, std), i, arg);
            }
            else {

                DEBUG ("0x%08x\n", val);

                GetLock(&pin_lock, PIN_ThreadId() + 1);
                time_stamp_g ++;
                ReleaseLock(&pin_lock);

                T_TYPE t1 = (T_TYPE)(t & ~T_OUT);
                if (t1 & T_DBLPTR)
                    t1 = (T_TYPE)(T_PTR | (t1 & ~T_DBLPTR));
                else if (t1 & T_PTR)
                    t1 = (T_TYPE)(t1 & ~T_PTR);
                else
                    DEBUG ("Impossible, T_OUT without T_DBLPTR or T_PTR\n");

                if (t1 != T_VOID) {
                    TAG tag = NewTAG(pc);
                    SetMTAG(arg, 4, tag, pc);
                    ResolveTAG(tag, t1);
                }
            }
        }
    }
}


VOID SyscallEntry(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    unsigned int syscall_num = PIN_GetSyscallNumber (ctxt, std);
    SetSyscallNum (syscall_num);
    const SYSCALL_ENTRY* syscall = FindSyscallByNum (syscall_num);
    if (syscall == NULL) return;

    ADDRINT eip = PIN_GetContextReg(ctxt, REG_INST_PTR);

    DEBUG ("entering syscall %-20s\n", syscall->syscall_name);

    fprintf (syscall_file, "0x%lx: %s %04lx(", eip, syscall->syscall_name, syscall_num);

    for (int i = 0; i < syscall->syscall_arg; i++) {
        fprintf (syscall_file, "arg 0x%lx, ", PIN_GetSyscallArgument (ctxt, std, i));

        SetSyscallArgTagType (ctxt, std, i, (T_TYPE)syscall->syscall_args[i]);
    }
           
    fprintf (syscall_file, ")\n");
}

VOID SyscallExit(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    ADDRINT eip = PIN_GetContextReg(ctxt, REG_INST_PTR);
    ADDRINT ret = PIN_GetSyscallReturn(ctxt, std);

    unsigned int syscall_num = GetSyscallNum();
    const SYSCALL_ENTRY* syscall = FindSyscallByNum (syscall_num);
    if (syscall == NULL) return;

    fprintf (syscall_file, "returns: 0x%lx\n", ret);

    TAG tag = NewTAG(eip);
    SetRTAG(REG_EAX, tag);
    ResolveTAG(tag, (T_TYPE)syscall->syscall_ret);
}
