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
#include "rewards.h"
#include "tls.h"
#include "shadow_mem.h"
#include "tag.h"

struct Heap {
    size_t size;
    ADDRINT pc;
    Heap(size_t __size = 0, ADDRINT __pc = 0) : size(__size), pc (__pc) {}
};

static ShadowMemory<Heap> shadow_heap;

static void SetShadowHeap(ADDRINT addr, ADDRINT size, ADDRINT eip)
{
    for (ADDRINT i = addr, e = addr + size; i < e; ++i, --size)
        shadow_heap.set (i, Heap(size, eip));

    ClrTAG(addr, size);
}

static void ClrShadowHeap(ADDRINT addr)
{
    ADDRINT size = shadow_heap.get(addr).size;

    for (ADDRINT i = addr, e = addr + size; i < e; ++i)
        shadow_heap.set (i, Heap());

    ClrTAG(addr, size);
}

/* ===================================================================== */
/* Analysis routines for malloc family functions                         */
/* ===================================================================== */
VOID MallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp)
{
    if (ret == NULL)
        return;

    uint32_t eip = 0, malloc_size = 0;
    PIN_SafeCopy(&eip, (ADDRINT*)esp, 4); 
    PIN_SafeCopy(&malloc_size, (ADDRINT*)(esp+4), 4); 

//    if (eip < 0x40000000)	//tracking the user level heap allocation
    if (IsInCodeSpace(eip))
    {
        TRACE ("HM 0x%08x 0x%08x 0x%08x\n", ret, malloc_size, eip);
        SetShadowHeap(ret, malloc_size, eip);
    }
}

VOID ReallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp)
{
    if (ret == NULL)
        return;

    uint32_t eip = 0, realloc_ptr = 0, realloc_size = 0;
    PIN_SafeCopy(&eip, (ADDRINT*)esp, 4); 
    PIN_SafeCopy(&realloc_ptr, (ADDRINT*)(esp+4), 4); 
    PIN_SafeCopy(&realloc_size, (ADDRINT*)(esp+8), 4); 

    ClrShadowHeap(realloc_ptr);

//    if (eip < 0x40000000)	//tracking the user level heap allocation
    if (IsInCodeSpace(eip))
    {
        TRACE ("HR 0x%08x 0x%08x 0x%08x\n", ret, realloc_size, eip);
        DEBUG ("AFTER %x realloc size %d ptr %p\n", ret, realloc_ptr, realloc_size);

        SetShadowHeap(ret, realloc_size, eip);
    }
}

VOID CallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp)
{
    if (ret == NULL)
        return;

    uint32_t eip = 0, calloc_size = 0, calloc_nmem = 0;
    PIN_SafeCopy(&eip, (ADDRINT*)esp, 4); 
    PIN_SafeCopy(&calloc_nmem, (ADDRINT*)(esp+4), 4); 
    PIN_SafeCopy(&calloc_size, (ADDRINT*)(esp+8), 4); 

//    if (eip < 0x40000000)	//tracking the user level heap allocation
    if (IsInCodeSpace(eip))
    {
        TRACE ("HC 0x%08x 0x%08x 0x%08x 0x%08x\n", ret, calloc_nmem, calloc_size, eip);
        DEBUG ("AFTER calloc calloc_nmem %d, calloc_size %d\n", calloc_nmem, calloc_size);

        SetShadowHeap(ret, calloc_size * calloc_nmem, eip);
    }
}

VOID FreeBefore(THREADID tid, ADDRINT ptr)
{
    TRACE ("HF 0x%08x\n", ptr);

    ClrShadowHeap(ptr);
}

bool IsInHeapSpace(ADDRINT addr)
{
    return (shadow_heap.get(addr).size > 0);
}

ADDRINT GetHeapAllocSite(ADDRINT addr)
{
    return shadow_heap.get(addr).pc;
}

ADDRINT GetHeapAllocSize(ADDRINT addr)
{
    return shadow_heap.get(addr).size;
}
