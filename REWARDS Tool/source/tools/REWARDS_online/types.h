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
#ifndef _TYPES_H
#define _TYPES_H

#define MEM_CHUNK       0x10000000
#define POINTER_BIT     0x20000000
#define ARRAY_BIT       0x40000000
#define STRUCT_BIT      0x80000000

#define RECV_BIT        0x08000000
#define RECV_FROM_BIT   0x04000000
#define READ_BIT        0x02000000

#define MALLOC_BIT      0x01000000
#define REALLOC_BIT     0x00800000
#define CALLOC_BIT      0x00400000
#define RTLALLOC_BIT    0x00200000
#define RTLREALLOC_BIT  0x00100000

#define DBL_POINTER_BIT 0x00080000
#define OUT_PARAM_BIT   0x00040000
#define IN_PARAM_BIT    0x00020000

#define TYPE_BIT_MASK   0x0000ffff

enum c_program_type_t {
    T_UNKNOWN = 0,
    T_CONFLICT,
    T_8,
    T_16,
    T_32,
    T_64,

    T_ALLOC_PTR,    // alloc returned pointer
    T_ALLOC_TARGET, // alloc returned pointer
    T_FUNCPTR,      // function pointer
    T_ESP,	    // stack pointer
    T_EBP,	    // Stack frame pointer
    T_RET_ADDR,
    T_STACK_PTR,

#include "typeenum.def"

    T_LAST_TYPE,

    T_RTLREALLOC_BIT = RTLREALLOC_BIT,
    T_RTLALLOC_BIT = RTLALLOC_BIT,
    T_CALLOC_BIT = CALLOC_BIT,
    T_REALLOC_BIT = REALLOC_BIT,
    T_MALLOC_BIT = MALLOC_BIT,

    T_READ_BIT = READ_BIT,
    T_RECV_FROM_BIT = RECV_FROM_BIT,
    T_RECV_BIT = RECV_BIT,

    T_MEM_CHUNK = MEM_CHUNK,
    T_PTR = POINTER_BIT,
    T_ARRAY = ARRAY_BIT,
    T_STRUCT = STRUCT_BIT,
    T_DBLPTR = DBL_POINTER_BIT,
    T_OUT = OUT_PARAM_BIT,
    T_IN = IN_PARAM_BIT,
};

#define T_VOIDPTR ((T_TYPE)(T_VOID | T_PTR))

typedef enum c_program_type_t T_TYPE;

struct TYPEINFO {
    unsigned int type;
    char name[64];
    unsigned int size;
};

struct FIELDINFO {
    unsigned int type;
    unsigned int pos;
    unsigned int size;
};

struct STRUCTINFO {
    unsigned int type;
    unsigned int fld;
    FIELDINFO flds[50];
};

const char *GetTypeNameByType(T_TYPE t);

typedef struct _TLS* TLS;

void ResolveStructType(TLS tls, T_TYPE t, unsigned int virt_addr);
void ResolveStructTypeArg(TLS tls, T_TYPE t, unsigned int esp);

T_TYPE Generalize(T_TYPE);

T_TYPE Dereference(T_TYPE);
T_TYPE Reference(T_TYPE);

#endif
