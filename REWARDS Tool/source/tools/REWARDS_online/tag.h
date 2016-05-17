#ifndef _TAG_H_
#define _TAG_H_

#include "pin.H"
#include "types.h"
#include <vector>
#include <set>

struct Memory {
    int type;
    ADDRINT address;
    ADDRINT desc1;
    ADDRINT desc2;

    Memory(): type(0), address(0), desc1(0), desc2(0) {}
    Memory(ADDRINT __address, ADDRINT __pc);
};


struct TYPE {
    set<T_TYPE> type;
    int distance;

    TYPE(): distance(2) {}
    bool valid() { return !type.empty(); }
};


struct _TAG;

enum REL_TYPE {
    REL_NONE = 0,
    REL_EQ,
    REL_ADD,
    REL_SUB,
    REL_COMBINE,
    REL_DEREFERENCE,
    REL_REFERENCE,
};

struct _REL {
    REL_TYPE reltype;
    _TAG*  tag;

    _REL(REL_TYPE, _TAG*);
    void Update(T_TYPE t, int d);
};

struct _TAG {
#ifdef WITH_TRACE
    ADDRINT pc;
    ADDRINT timestamp;
    ADDRINT immediate;
    ADDRINT is_immediate;
#endif

    TYPE type;
    BOOL dumped;
    std::vector<_TAG*> children;
    vector<_REL> rels;

    _TAG(ADDRINT pc);
    _TAG(ADDRINT pc, ADDRINT imm);

    BOOL Resolve(T_TYPE t, int d);

//    void resolve(T_TYPE type);
//    virtual void update(int depth);
//    virtual void dump();
//    virtual bool _resolve(_TAG* p, T_TYPE type, int depth);
};

typedef struct _TAG *TAG;

TAG  NewTAG (ADDRINT pc);
TAG  NewTAG (ADDRINT pc, ADDRINT imm);

void ClrTAG(ADDRINT addr, ADDRINT size);

void SetMTAG (ADDRINT addr, ADDRINT size, TAG tag, ADDRINT pc);
TAG  GetMTAG (ADDRINT addr, ADDRINT size, ADDRINT pc);
TAG  GetOrNewMTAG (ADDRINT addr, ADDRINT size, ADDRINT pc);
void SetRTAG (REG reg, TAG tag);
TAG  GetRTAG (REG reg, ADDRINT pc);
TAG  GetOrNewRTAG (REG reg, ADDRINT pc);

TAG  UnionTAG (ADDRINT pc, TAG tag1, TAG tag2, REL_TYPE reltype);
TAG  PtrATAG (ADDRINT pc, TAG rtag, TAG mtag);
TAG  PtrBTAG (ADDRINT pc, TAG rtag, TAG mtag);
void ResolveTAG (TAG tag, T_TYPE type);

#endif
