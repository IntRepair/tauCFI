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
#ifndef __UTILITY_H
#define __UTILITY_H

#include "inttypes.h"

#define PAGE_SIZE 0x10000
#define PAGE_NUM  0x10000

template <class T>
class ShadowMemory {
    struct SecMap {
        T byte[PAGE_SIZE];
        SecMap()
        {
            for (int i = 0; i < PAGE_SIZE; ++i)
                byte[i] = T();
        }
    };

    SecMap* map[PAGE_NUM];
    SecMap* empty_map;

public:
    ShadowMemory()
    {
        empty_map = new SecMap();
        for (int i = 0; i < PAGE_NUM; ++i)
            map[i] = empty_map;
    }

    void set(uint32_t addr, const T& v)
    {
        if (map[addr >> 16] == empty_map)
            map[addr >> 16] = new SecMap();
        map[addr >> 16]->byte[addr & 0xffff] = v;
    }

    const T& get(uint32_t addr)
    {
        return map[addr >> 16]->byte[addr & 0xffff];
    }
};

#endif
