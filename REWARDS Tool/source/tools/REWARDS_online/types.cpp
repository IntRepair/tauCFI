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

#include "inttypes.h"
#include "pin.H"
#include "types.h"
#include "tls.h"
#include "tag.h"

TYPEINFO types[] = {
{T_UNKNOWN,                                    "UNKNOWN",                                    0  },
{T_CONFLICT,                                   "CONFLICT",                                   0  },
{T_8,                                          "8bit",                                       0  },
{T_16,                                         "16bit",                                      0  },
{T_32,                                         "32bit",                                      0  },
{T_64,                                         "64bit",                                      0  },

{T_ALLOC_PTR,                                  "alloc ptr*",                                 4  },
{T_ALLOC_TARGET,                               "alloc target",                               4  },
{T_FUNCPTR,                                    "void (*f)()",                                4  },
{T_ESP,                                        "esp_t",                                      4  },
{T_EBP,                                        "ebp_t",                                      4  },
{T_RET_ADDR,                                   "ret_addr_t",                                 4  },
{T_STACK_PTR,                                  "stack_ptr",                                  4  },

#include "type.def"
};

STRUCTINFO structs[] = {
#include "struct.def"
};

static void SetFieldType(ADDRINT addr, T_TYPE t, ADDRINT size)
{
    static uint32_t simulated_pc = 0;

    DEBUG (" SetFieldType called\n");

//  turn off the backward propagation for resolved arg, but keeps forwared
    TAG tag = NewTAG(++simulated_pc);
    SetMTAG( addr, size, tag, 0 );
    ResolveTAG( tag, t );
}

const char *GetTypeNameByType(T_TYPE t)
{
    T_TYPE tmp = (T_TYPE) ((unsigned int)t & TYPE_BIT_MASK);
    if (tmp > T_LAST_TYPE)
        return NULL;
    else
        return types[tmp].name;
}

void ResolveStructTypeArg(TLS tls, T_TYPE t, uint32_t esp)
{
//    ResolveStructType(t, *(uint32_t *) esp);
}

void ResolveStructType(TLS tls, T_TYPE t, uint32_t virt_addr)
{
    if (t <= T___STRUCT0__ || t >= T_LAST_TYPE)
        return;

    STRUCTINFO *info = &(structs[t - T___STRUCT0__ - 1]);
    if (info->type != t)
        return;

    for (unsigned int i = 0; i < info->fld; i++) {
        SetFieldType(virt_addr + info->flds[i].pos,(T_TYPE)info->flds[i].type, info->flds[i].size);
    }
}

T_TYPE Generalize(T_TYPE t)
{
    switch (t) {
    case T_ESP:
    case T_EBP:
    case T_STACK_PTR:
        return T_STACK_PTR;
    case T_FUNCPTR:
    case T_RET_ADDR:
    case T_PARRAY_INFO:
        return T_VOIDPTR;
    case T_DOUBLE:
        return T_DOUBLE;
    default:
        break;
    }

    if ((t & T_PTR) || (t & T_DBLPTR))
        return T_VOIDPTR;

    if (t < T___STRUCT0__) {
        switch (types[t].size) {
        case 0: return T_UNKNOWN;
        case 1: return T_CHAR;
        case 2: return T_SHORT_INT;
        case 4: return T_INT;
        case 8: return T_LONG_LONG_UNSIGNED_INT;
        default: return T_UNKNOWN;
        }
    }
    else {
        return t;
    }
}

T_TYPE Dereference(T_TYPE t)
{
    if (t == T_VOIDPTR)
        return T_UNKNOWN;
    if (t & T_PTR)
        return (T_TYPE)(t & (~T_PTR));
    if (t & T_DBLPTR)
        return (T_TYPE)(t & (~T_DBLPTR) | T_PTR);
    return T_UNKNOWN;
}

T_TYPE Reference(T_TYPE t)
{
    if (t == T_UNKNOWN)
        return T_UNKNOWN;
    if (t & T_PTR)
        return (T_TYPE)(t & (~T_PTR) | T_DBLPTR);
    if (t & T_DBLPTR)
        return T_UNKNOWN;
    return (T_TYPE)(t | T_PTR);
}
