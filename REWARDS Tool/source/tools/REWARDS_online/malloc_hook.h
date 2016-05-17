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
#ifndef __MALLOC_HOOK__
#define __MALLOC_HOOK__

VOID MallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp);
VOID ReallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp);
VOID CallocAfter(THREADID tid, ADDRINT ret, ADDRINT esp);
VOID FreeBefore(THREADID tid, ADDRINT ptr);
VOID FreeObjectBefore(THREADID tid, ADDRINT ptr, ADDRINT esp);

#endif
