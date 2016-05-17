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

#ifndef _REWARDS_H_
#define _REWARDS_H_

#include "pin.H"
#include "inttypes.h"
#include <stdio.h>
#include <map>

//typedef struct __global_space_list {
//    unsigned int start;
//    unsigned int end;
//    char name[64];
//    struct __global_space_list *next;
//} GlobalSpaceList;

typedef struct __space_list {
    unsigned int start;
    unsigned int end;
    struct __space_list *next;
} SpaceList;

void InsertToSpaceList(SpaceList**, ADDRINT start, ADDRINT end);
bool IsInSpaceList(SpaceList**, ADDRINT addr);

const char *ParseImageName(const char *imagename);
bool IsInDataSpace(ADDRINT addr);
bool IsInCodeSpace(ADDRINT addr);
bool IsInHeapSpace(ADDRINT addr);
bool IsInStackSpace(ADDRINT addr);
ADDRINT GetHeapAllocSite(ADDRINT addr);
ADDRINT GetHeapAllocSize(ADDRINT addr);

VOID LoadStaticlinkFunction(string filename);
VOID HandleStaticLinkFunction(INS ins);
VOID ImageLoad(IMG img, VOID *v);
VOID ImageRoutineReplace(IMG img, VOID *v);

void setup_hook();
void InstrumentINS(INS ins, VOID*);
void Instrument_Trace(TRACE trace);

VOID SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
VOID SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);

//extern FILE* log;
//extern FILE* semantic_file;
extern FILE* syscall_file;
extern FILE* fptrace;
extern FILE* fperr;

extern char filename_prefix[32];
extern char ImageName[128];

extern uint32_t time_stamp_g;

VOID UpdateTimeStamp();

extern PIN_LOCK fptrace_lock;

#define ERROR(...) fprintf(fperr, __VA_ARGS__)

#define BEGIN_PRINT() GetLock(&fptrace_lock, PIN_ThreadId()+1)
#define END_PRINT() ReleaseLock(&fptrace_lock)
#define PRINT(...) do { fprintf(fptrace, __VA_ARGS__); DEBUG(__VA_ARGS__); } while(0)

#endif
