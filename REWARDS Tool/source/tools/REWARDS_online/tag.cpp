#include <assert.h>
#include "pin.H"
#include "rewards.h"
#include "tls.h"
#include "tag.h"
#include "shadow_mem.h"
#include "types.h"

static ShadowMemory<TAG> shadow_memory;

template<class M, class V>
bool member(const M& m, const V& v)
{ return m.find(v) != m.end(); }

static _TAG invalid_tag(0);

#define IS_PTR(t) ((T_PTR & (t)) || (T_DBLPTR & (t)))
#define IS_NUMBER(t) ((t) == T_CHAR || (t) == T_SHORT_INT || (t) == T_INT || (t) == T_LONG_LONG_UNSIGNED_INT)


Memory::Memory(ADDRINT __address, ADDRINT __pc)
    : type(0), address(__address), desc1(0), desc2(0)
{
    if (IsInDataSpace(__address)) {
        type = 1; 
    }
    if (IsInHeapSpace(__address)) {
        type = 2;
        desc1 = GetHeapAllocSite(__address);
        desc2 = GetHeapAllocSize(__address);
    }
    if (IsInStackSpace(__address) && IsInCodeSpace(__pc)) {
        type = 3;

        TLS tls = GetTLS();
        if (!tls->callstack.empty()) {
            desc1 = tls->callstack.back().funaddr;
            desc2 = tls->callstack.back().esp - __address;
        }
    }
}



_TAG::_TAG(ADDRINT _pc) :
#ifdef WITH_TRACE
    pc(_pc), timestamp(time_stamp_g), immediate(0), is_immediate(0),
#endif
    dumped(FALSE)
{ }

_TAG::_TAG(ADDRINT _pc, ADDRINT _imm) :
#ifdef WITH_TRACE
    pc(_pc), timestamp(time_stamp_g), immediate(_imm), is_immediate(1),
#endif
    dumped(FALSE)
{ }

BOOL
_TAG::Resolve(T_TYPE t, int d)
{
    DEBUG ("_X %08x %08x %d\n", this, t, d);

    if (d > 3) return FALSE;
    if (t == T_UNKNOWN) return FALSE;
    if (this == &invalid_tag) return FALSE;
    if (d == 0 && (t == T_8 || t == T_16 || t == T_32 || t == T_64)) return FALSE;

    if (d == 0 && !(t == T_VOIDPTR || t == (T_VOID | T_DBLPTR))) {
        if (type.distance != 0) {
            type.distance = 0;
            type.type.clear();
        }
    }
    else if (d != 0) {
        if (type.distance == 0) {
            return FALSE;
        }
        type.distance = 1;
    }

    if (member(type.type, t)) return FALSE;
    type.type.insert (t);


    DEBUG ("X %08x %08x %d\n", this, t, d);
    TRACE ("X 0x%08x 0x%08x 0x%08x 0x%08x\n", pc, timestamp, t, this);

    if (dumped)
        PRINT ("X %08x %08x %d\n", this, t, type.distance);

    for (vector<_REL>::iterator 
            i = rels.begin(), e = rels.end(); i != e; ++i) {
//        if (type.distance == 0 || type.type.size() == 1)
            i->Update(t, d+1);
    }

    return TRUE;
}


_REL::_REL(REL_TYPE _reltype, TAG _tag)
    : reltype(_reltype), tag(_tag) {}

VOID
_REL::Update(T_TYPE t, int d)
{
    DEBUG(" update %08x %d %08x %d\n", tag, reltype, t, d);

    switch (reltype) {
    case REL_NONE:
        break;
    case REL_EQ:
        tag->Resolve(t, d-1);
    case REL_ADD:
        if (IS_PTR(t))
            tag->Resolve(T_INT, d);
        else
            tag->Resolve(t, d);
        break;
    case REL_SUB:
        //tag->Resolve(t, d);
        break;
    case REL_COMBINE:
        tag->Resolve(t, d);
        break;
    case REL_DEREFERENCE:
        tag->Resolve(Dereference(t), d);
        break;
    case REL_REFERENCE:
        tag->Resolve(Reference(t), d);
        break;
    }
}
static inline VOID
DumpTAG(TAG tag)
{
    if (tag == &invalid_tag) return;
    for (set<T_TYPE>::iterator
            i = tag->type.type.begin(), e = tag->type.type.end(); i != e; ++i) {
        if (*i != 0)
            PRINT ("X %08x %08x %d\n", tag, *i, tag->type.distance);
    }
}

VOID
RelateTAG(TAG t1, TAG t2, REL_TYPE reltype)
{
    if (t1 == NULL || t1 == &invalid_tag) return;
    if (t2 == NULL || t2 == &invalid_tag) return;
    if (t1 == t2) return;

    _REL rel(reltype, t2);
    t1->rels.push_back(rel);

    for (set<T_TYPE>::iterator 
            i = t1->type.type.begin(), e = t1->type.type.end(); i != e; ++i) {
        if (t1->type.distance == 0 || t1->type.type.size() < 3)
            rel.Update(*i, t1->type.distance + 1);
    }
}

VOID ClrTAG(ADDRINT addr, ADDRINT size)
{
    for (ADDRINT i = addr, e = addr + size; i < e; i++)
        shadow_memory.set (i, 0);
}

TAG NewTAG (ADDRINT pc)
{
    TAG tag = new _TAG(pc);
    DEBUG ("D %08x\n", tag);
    TRACE ("D 0x%08x 0x%08x\n", tag->pc, tag->timestamp);
    return tag;
}

TAG NewTAG (ADDRINT pc, ADDRINT imm)
{
    TAG tag = new _TAG(pc, imm);
    DEBUG ("D %08x\n", tag);
    TRACE ("D 0x%08x 0x%08x\n", tag->pc, tag->timestamp);
    return tag;
}

TAG UnionTAG (ADDRINT pc, TAG tag1, TAG tag2, REL_TYPE reltype)
{
    if (tag1 == &invalid_tag || tag2 == &invalid_tag) return &invalid_tag;
    if (tag1 == tag2) return tag1;

    TRACE ("U 0x%08x 0x%08x 0x%08x 0x%08x\n",
           tag1->pc, tag1->timestamp, tag2->pc, tag2->timestamp);
    DEBUG ("U %08x %08x\n", tag1, tag2);
    RelateTAG(tag1, tag2, reltype);
    RelateTAG(tag2, tag1, reltype);
    return tag1;
}

TAG PtrATAG (ADDRINT pc, TAG rtag, TAG mtag)
{
    if (rtag == &invalid_tag || mtag == &invalid_tag) return &invalid_tag;
    DEBUG("UP %08x %08x\n", rtag, mtag);
    RelateTAG(rtag, mtag, REL_DEREFERENCE);
    return rtag;
}

TAG PtrBTAG (ADDRINT pc, TAG rtag, TAG mtag)
{
    if (rtag == &invalid_tag || mtag == &invalid_tag) return &invalid_tag;
    DEBUG("UP %08x %08x\n", rtag, mtag);
    RelateTAG(mtag, rtag, REL_REFERENCE);
    return mtag;
}

void ResolveTAG(TAG tag, T_TYPE type)
{
    if (tag == &invalid_tag) return;

    BEGIN_PRINT();
    tag->Resolve(type, 0);
    END_PRINT();
}

void SetMTAG (ADDRINT addr, ADDRINT size, TAG tag, ADDRINT pc)
{
    if (tag == &invalid_tag) return;

    TRACE ("M 0x%08x 0x%08x 0x%08x 0x%08x w\n", addr, size, tag->pc, tag->timestamp);

#ifdef WITH_TRACE
    if (tag->is_immediate)
        TRACE ("p 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                addr, size, tag->pc, tag->timestamp, tag->immediate);
#endif

    Memory mem (addr, pc);
//    DEBUG ("M %08x %08x %d %08x %08x %d\n", tag, mem.address, mem.type, mem.desc1, mem.desc2, size);
    DEBUG ("Set MEM %08x [ %08x ] (%08x %08x %08x %d)\n", addr, tag, mem.type, mem.desc1, mem.desc2, size);

    TAG oldtag = shadow_memory.get (addr);
    if (!oldtag) 
        shadow_memory.set (addr, tag);

    switch (mem.type) {
    case 0: 
        return;
    case 1:
    case 2:
    case 3:
        DEBUG("UE %08x %08x\n", oldtag, tag);
        RelateTAG(oldtag, tag, REL_EQ);
        RelateTAG(tag, oldtag, REL_EQ);
        break;
        break;
    }

    switch(size) {
    case 1: tag->type.type.insert (T_8);  break;
    case 2: tag->type.type.insert (T_16); break;
    case 4: tag->type.type.insert (T_32); break;
    case 8: tag->type.type.insert (T_64); break;
    default: break;
    }

    BEGIN_PRINT();
    PRINT ("M %08x %08x %d %08x %08x\n", tag, mem.address, mem.type, mem.desc1, mem.desc2);

//    if (tag->type.valid() && !tag->dumped) {
    if (!tag->dumped) {
        DumpTAG(tag);
        tag->dumped = true;
    }

    END_PRINT();
}

TAG  GetMTAG (ADDRINT addr, ADDRINT size, ADDRINT pc)
{
    TAG tag = shadow_memory.get(addr);

    DEBUG ("Get MEM %08x [ %08x ]\n", addr, tag);

    return tag ? tag : &invalid_tag;
}

TAG  GetOrNewMTAG (ADDRINT addr, ADDRINT size, ADDRINT pc)
{
    TAG tag = shadow_memory.get(addr);
    if (tag == NULL) {
        tag = NewTAG(pc);
        SetMTAG (addr, size, tag, pc); 
    }

    TRACE ("M 0x%08x 0x%08x 0x%08x 0x%08x r\n", addr, size, tag->pc, tag->timestamp);
    
    DEBUG ("Get MEM %08x [ %08x ]\n", addr, tag);

    Memory mem (addr, pc);
    if (mem.type == 1) { // if global variable
        TLS tls = GetTLS();

        if (!tls->callstack.empty() && IsInCodeSpace(pc)) {
            ADDRINT v = 0;
            ASSERT(size <= sizeof(v), "size(" + decstr(size) + ") > " + decstr(sizeof(v)) + " at pc : " + hexstr(pc) + "\n");

            PIN_SafeCopy(&v, (VOID*)addr, size);

            PRINT ("G %08x %08x %08x\n", tls->callstack.back().funaddr, addr, v);
        }
    }

    return tag;
}



void SetRTAG (REG reg, TAG tag)
{
#ifdef WITH_TRACE
    DEBUG ("Set REG %s [ 0x%08x 0x%08x ]\n", REG_StringShort(reg).c_str(), tag->pc, tag->timestamp);
#endif
    DEBUG ("Set REG %s [ %08x ]\n", REG_StringShort(reg).c_str(), tag);

    TLS tls = GetTLS();

    if (tag != &invalid_tag) {
        tls->regTagMap[reg] = tag;
        switch (reg) {
            case REG_EAX: case REG_AX: case REG_AH: case REG_AL:
                tls->regTagMap[REG_EAX] = tls->regTagMap[REG_AX] = 
                tls->regTagMap[REG_AH]  = tls->regTagMap[REG_AL] =  tag;
                break;
            case REG_EBX: case REG_BX: case REG_BH: case REG_BL:
                tls->regTagMap[REG_EBX] = tls->regTagMap[REG_BX] = 
                tls->regTagMap[REG_BH]  = tls->regTagMap[REG_BL] =  tag;
                break;
            case REG_ECX: case REG_CX: case REG_CH: case REG_CL:
                tls->regTagMap[REG_ECX] = tls->regTagMap[REG_CX] = 
                tls->regTagMap[REG_CH]  = tls->regTagMap[REG_CL] =  tag;
                break;
            case REG_EDX: case REG_DX: case REG_DH: case REG_DL:
                tls->regTagMap[REG_EDX] = tls->regTagMap[REG_DX] = 
                tls->regTagMap[REG_DH]  = tls->regTagMap[REG_DL] =  tag;
                break;
            case REG_ESI: case REG_SI:
                tls->regTagMap[REG_ESI] = tls->regTagMap[REG_SI] = tag;
                break;
            case REG_EDI: case REG_DI:
                tls->regTagMap[REG_EDI] = tls->regTagMap[REG_DI] = tag;
                break;
        }
    }

    if (!tls->callstack.empty()) {
        tls->regRTN[reg] = tls->callstack.back().funaddr;
        switch (reg) {
            case REG_EAX: case REG_AX: case REG_AH: case REG_AL:
                tls->regRTN[REG_EAX] = tls->regRTN[REG_AX] = 
                tls->regRTN[REG_AH]  = tls->regRTN[REG_AL] =  tls->callstack.back().funaddr;
                break;
            case REG_EBX: case REG_BX: case REG_BH: case REG_BL:
                tls->regRTN[REG_EBX] = tls->regRTN[REG_BX] = 
                tls->regRTN[REG_BH]  = tls->regRTN[REG_BL] =  tls->callstack.back().funaddr;
                break;
            case REG_ECX: case REG_CX: case REG_CH: case REG_CL:
                tls->regRTN[REG_ECX] = tls->regRTN[REG_CX] = 
                tls->regRTN[REG_CH]  = tls->regRTN[REG_CL] =  tls->callstack.back().funaddr;
                break;
            case REG_EDX: case REG_DX: case REG_DH: case REG_DL:
                tls->regRTN[REG_EDX] = tls->regRTN[REG_DX] = 
                tls->regRTN[REG_DH]  = tls->regRTN[REG_DL] =  tls->callstack.back().funaddr;
                break;
            case REG_ESI: case REG_SI:
                tls->regRTN[REG_ESI] = tls->regRTN[REG_SI] = tls->callstack.back().funaddr;
                break;
            case REG_EDI: case REG_DI:
                tls->regRTN[REG_EDI] = tls->regRTN[REG_DI] = tls->callstack.back().funaddr;
                break;
        }
    }
}

TAG  GetRTAG (REG reg, ADDRINT pc)
{
    TAG tag = GetTLS()->regTagMap[reg];
    return tag ? tag : &invalid_tag;
}

TAG  GetOrNewRTAG (REG reg, ADDRINT pc)
{
    TAG tag = GetTLS()->regTagMap[reg];
    if (tag == NULL) {
        tag = NewTAG(pc);
        SetRTAG (reg, tag);
    }

#ifdef WITH_TRACE
    DEBUG ("Get REG %s [ 0x%08x 0x%08x ]\n", REG_StringShort((REG)reg).c_str(), tag->pc, tag->timestamp);
#endif

    DEBUG ("Get REG %s [ %08x ]\n", REG_StringShort(reg).c_str(), tag);

    return tag;
}
