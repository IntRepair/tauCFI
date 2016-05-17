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
#include "malloc_hook.h"
#include "types.h"
#include "tag.h"
#include "tls.h"
#include <map>

struct FUNC_ENTRY {
    const char* function_name;
    unsigned int function_ret;
    unsigned int function_arg;
    unsigned int function_args[20];
};

FUNC_ENTRY functions[] = {
#include "apicall.def"
};

static VOID FunctionEntry(void* _func, ADDRINT esp, ADDRINT pc)
{
    const FUNC_ENTRY* func = (const FUNC_ENTRY*)_func;

    DEBUG ("entering %-20s\n", func->function_name);

    ADDRINT addr = esp;
    for (unsigned int i = 0; i < func->function_arg; i++) {
        addr += 4; // esp + 4*(i + 1)
        ResolveTAG (GetOrNewMTAG(addr, 4, pc), (T_TYPE)func->function_args[i]);
    }
//        SetTaintForMemoryWithType (tls, esp + 4 * (i + 1), (T_TYPE)func->function_args[i]);
}

static VOID FunctionExit (void* _func, ADDRINT pc, ADDRINT esp/*, ADDRINT eax*/)
{
    const FUNC_ENTRY* func = (const FUNC_ENTRY*)_func;

    DEBUG ("exiting %-20s\n", func->function_name);

    if (func->function_ret != T_VOID) {
        TAG tag = NewTAG(pc);
        SetRTAG(REG_EAX, tag);
        ResolveTAG (tag, (T_TYPE)func->function_ret);
    }

    ClrTAG(esp+4, 4*func->function_arg);
}

VOID ImageRoutineReplace(IMG img, VOID * v)
{
    RTN insertRtn;
    for (int i = 0; i < sizeof(functions) / sizeof(FUNC_ENTRY); i++) {
        insertRtn = RTN_FindByName (img, functions[i].function_name);
        if (RTN_Valid (insertRtn)) {
            RTN_Open (insertRtn);

            RTN_InsertCall (insertRtn, IPOINT_BEFORE,
                            (AFUNPTR) FunctionEntry,
                            IARG_PTR,       &functions[i],
                            IARG_REG_VALUE, REG_ESP, 
                            IARG_INST_PTR,
                            IARG_END);

            RTN_InsertCall (insertRtn, IPOINT_AFTER,
                            (AFUNPTR) FunctionExit,
                            IARG_PTR,       &functions[i],
                            IARG_INST_PTR,
                            IARG_REG_VALUE, REG_ESP,
                            /*IARG_FUNCRET_EXITPOINT_VALUE,*/
                            IARG_END);

            RTN_Close (insertRtn);
        }
    }

    // Instrument the malloc() and free() functions.  Print the input argument
    // of each malloc() or free(), and the return value of malloc().

    // Find the malloc() function.
    RTN mallocRtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(mallocRtn)) {
        RTN_Open(mallocRtn);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR) MallocAfter,
                       IARG_THREAD_ID,
                       IARG_FUNCRET_EXITPOINT_VALUE,
                       IARG_REG_VALUE, REG_ESP, IARG_END);
        RTN_Close(mallocRtn);
    }

    RTN reallocRtn = RTN_FindByName(img, "realloc");
    if (RTN_Valid(reallocRtn)) {
        RTN_Open(reallocRtn);
        RTN_InsertCall(reallocRtn, IPOINT_AFTER, (AFUNPTR) ReallocAfter,
                       IARG_THREAD_ID,
                       IARG_FUNCRET_EXITPOINT_VALUE,
                       IARG_REG_VALUE, REG_ESP, IARG_END);
        RTN_Close(reallocRtn);
    }

    RTN callocRtn = RTN_FindByName(img, "calloc");
    if (RTN_Valid(callocRtn)) {
        string image_name = IMG_Name(img);

        if (image_name != "/lib/ld-linux.so.2") {
            //whiltelist ld-linux.so because calloc at there is not just to allocate memory

            RTN_Open(callocRtn);
            RTN_InsertCall(callocRtn, IPOINT_AFTER, (AFUNPTR) CallocAfter,
                           IARG_THREAD_ID,
                           IARG_FUNCRET_EXITPOINT_VALUE,
                           IARG_REG_VALUE, REG_ESP, IARG_END);
            RTN_Close(callocRtn);
        }
    }

    // Find the free() function.
    RTN freeRtn = RTN_FindByName(img, "free");
    if (RTN_Valid(freeRtn)) {
        RTN_Open(freeRtn);
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR) FreeBefore,
                       IARG_THREAD_ID,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(freeRtn);
    }
}

map<ADDRINT, FUNC_ENTRY*> statically_linked_functions;

VOID LoadStaticlinkFunction(string filename)
{
    if (filename == "") return;

    map<string, FUNC_ENTRY*> functions_map;
    for (int i = 0; i < sizeof(functions) / sizeof(FUNC_ENTRY); i++)
        functions_map[functions[i].function_name] = &functions[i];

    FILE *fp = fopen(filename.c_str(), "r");
    char name[512];
    ADDRINT addr1, addr2;

    while (fscanf(fp, "%s %x %x", name, &addr1, &addr2) == 3) {
        if (functions_map[name]) {
            LOG(string(name) + " " + hexstr(addr1) + "\n");
            statically_linked_functions[addr1] = functions_map[name];
        }
        else if (name[0] == '_' && functions_map[name+1]) {
            LOG("a: " + string(name) + " " + hexstr(addr1) + "\n");
            statically_linked_functions[addr1] = functions_map[name+1];
        }
    }

    fclose(fp);
}

VOID HandleStaticLinkFunction(INS ins)
{
    ADDRINT addr = INS_Address(ins);
    FUNC_ENTRY* entry = statically_linked_functions[addr];

    if (entry == NULL) return;

    INS_InsertCall (ins, IPOINT_BEFORE, (AFUNPTR)FunctionEntry,
                    IARG_PTR, entry,
                    IARG_REG_VALUE, REG_ESP,
                    IARG_INST_PTR,
                    IARG_END);
}
