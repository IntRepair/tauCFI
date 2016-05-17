#ifndef TLS_H
#define TLS_H

#include "pin.H"
#include <list>
#include <utility>

struct CALLENTRY {
    ADDRINT callsite;
    ADDRINT funaddr;
    ADDRINT esp;
    ADDRINT param0;
    ADDRINT param1;
    ADDRINT retaddr;
    ADDRINT regparam;
    ADDRINT nparam;
};

typedef struct _TAG* TAG;
typedef struct _TLS {
    TAG regTagMap[(unsigned int) LEVEL_BASE::REG_LAST];
    ADDRINT regRTN[(unsigned int) LEVEL_BASE::REG_LAST];
    unsigned int syscall_number;
    FILE *fptrace;

    std::list<CALLENTRY> callstack;
    TAG exittag;
    ADDRINT exitvalue;
    ADDRINT retfunction;
} *TLS;

VOID InitTLS();
TLS GetTLS();
TLS NewTLS(THREADID tid);

void _TRACE (const char* format, ...);

#define TPRINT(...) _TRACE(__VA_ARGS__)

#ifdef WITH_TRACE
#define TRACE(...) _TRACE(__VA_ARGS__)
#else
#define TRACE(...) ((void)0)
#endif

#ifndef NDEBUG
extern KNOB <BOOL> KnobDebug;
#define DEBUG(...) if (KnobDebug) _TRACE(__VA_ARGS__)
#else
#define DEBUG(...) ((void)0)
#endif

#endif
