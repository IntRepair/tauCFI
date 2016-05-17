#include "pin.H"
#include "rewards.h"
#include "tls.h"
#include <stdarg.h>

static TLS_KEY key;

static VOID DestructTLS(void *);

VOID InitTLS()
{
    key = PIN_CreateThreadDataKey(DestructTLS);
}

TLS GetTLS()
{
    TLS tls = (TLS)PIN_GetThreadData(key, PIN_ThreadId());
    return tls;
}

TLS NewTLS(THREADID tid)
{
    TLS tls = new _TLS();
    memset(tls->regTagMap, 0, sizeof(tls->regTagMap));
    memset(tls->regRTN, 0, sizeof(tls->regRTN));
    tls->syscall_number = 0;

//    TLS tls = (TLS)malloc(sizeof(*tls));
//    memset (tls, 0, sizeof(*tls));

    char temp_file_name[256];
    sprintf (temp_file_name, "%s/alltrace.%d.out", filename_prefix, tid);
    tls->fptrace = fopen(temp_file_name, "w");

    tls->exitvalue = 0;
    tls->retfunction = 0;

    PIN_SetThreadData(key, tls, tid);
    return tls;
}

static VOID DestructTLS(void *p)
{
    TLS tls = (TLS)p;
    fclose(tls->fptrace);
    free(p);
}

//extern KNOB <BOOL> KnobTrace;
void _TRACE (const char* format, ...)
{
//    if (!KnobTrace) return;
    FILE *fp = GetTLS()->fptrace;

    va_list args; 
    va_start (args, format);
    vfprintf (fp, format, args);
    va_end   (args);
}
