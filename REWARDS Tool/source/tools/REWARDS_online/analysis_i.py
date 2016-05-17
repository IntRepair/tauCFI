#!/usr/bin/env python3

import sys
import os
from itertools import count, chain
from collections import defaultdict


MEM_CHUNK     = 0x10000000
POINTER_BIT   = 0x20000000
ARRAY_BIT     = 0x40000000
STRUCT_BIT    = 0x80000000

RECV_BIT      = 0x08000000
RECV_FROM_BIT = 0x04000000
READ_BIT      = 0x02000000

MALLOC_BIT    = 0x01000000
REALLOC_BIT   = 0x00800000
CALLOC_BIT    = 0x00400000
RTLALLOC_BIT  = 0x00200000
RTLREALLOC_BIT= 0x00100000

DBL_POINTER_BIT=0x00080000
OUT_PARAM_BIT = 0x00040000

TYPE_BIT_MASK = 0x0000ffff




class defaultlist(list):
    def __init__(self, factory):
        self._factory = factory
    def _auto_expand(self, index):
        l = len(self)
        if index >= l:
            self.extend([None] * (index - l + 1))
        for i in range(l, index + 1):
            super(defaultlist, self).__setitem__(i, self._factory())
    def __getitem__(self, index):
        self._auto_expand(index)
        return super(defaultlist, self).__getitem__(index)
    def __setitem__(self, index, value):
        self._auto_expand(index)
        super(defaultlist, self).__setitem__(index, value)

class Type:
    def __init__(self):
        self.typeset  = set() # empty type set
        self.distance = 99999 # max distance
    def addtype(self, typeenum, distance):
        if self.distance > distance:
            self.typeset = set([typeenum])
            self.distance = distance
        elif self.distance == distance:
            self.typeset.add (typeenum)

class Func:
    def __init__(self):
        self.sparams = set()    # set of (off, tag, val)
        self.rparams = set()    # set of (off, tag, val)
        self.rettype = set()    # set of (tag, val)
        self.globaldata = set() # set of (mem addr, val)
        self.stackimage = set() # set of ((par1 tag, par1 val), ...)

    def add_sparam(self, off, tag, val):
        if off % 4 != 0:
            return
        off = int(off/4)
        self.sparams.add ((off, tag, val))

    def add_rparam(self, off, tag, val):
        assert (off in [1,2,3,4])
        self.rparams.add ((off, tag, val))

    def add_return(self, tag, val):
        self.rettype.add ((tag, val))

    def add_globaldata(self, mem, val):
        self.globaldata.add ((mem, val))

    def add_stackimage(self, params):
        self.stackimage.add (tuple(params))

class TypeValue:
    def __init__(self):
        self.typeset = set()
        self.value = None
    def __bool__(self):
        return bool(self.typeset) or self.value is not None
    def update(self, o):
        self.typeset.update(o.typeset)
        self.value = o.value if self.value is None else self.value

class FuncResult:
    def __init__(self):
        self.sparams = defaultlist(TypeValue)   # off -> TypeValue
        self.rparams = defaultdict(TypeValue)   # off -> TypeValue
        self.rettype = TypeValue()
        self.globaldata = dict()                # mem addr -> value
    def update(self, r):
        for off,p in zip(count(0),r.sparams):
            self.sparams[off].update(p)
        for off,p in r.rparams.items():
            self.rparams[off].update(p)
        self.rettype.update(r.rettype)
        self.globaldata.update(r.globaldata)
    

def v_to_str(v):
    return "None" if v is None else "%08x" % v
def t_to_str(t):
    return " ".join(("%08x" % x for x in t))

class AnalysisResult:
    def __init__(self):
        self.data = defaultdict(set)          # key -> typeset
        self.heap = defaultdict(set)          # key -> typeset
        self.heap_address = defaultdict(set)  # key -> set of addresses (baseaddr, size)
        self.stack = defaultdict(lambda: defaultdict(set))  # key (func, off) -> typeset -> set of addresses
        self.func = defaultdict(FuncResult)   # key -> FuncResult

    def merge(self, r):
        for key,items in r.data.items():
            self.data[key].update(items)
        for key,items in r.heap.items():
            self.heap[key].update(items)
            self.heap_address[key].update (r.heap_address[key])
        for key,items in r.stack.items():
            for typeset,addrs in items.items():
                self.stack[key][typeset].update(addrs)
        for key,func in r.func.items(): 
            self.func[key].update(func)

    def PrettyPrint(self, filename):
        o = open(filename, "w")
        o.write ("== Data section ==\n") 
        o.write ("\n")
        o.write ("addresss: resolved type\n")
        for key,items in sorted(self.data.items()):
            o.write ("%08x: %s\n" % (key, type_to_str(namesof(items))))
        o.write ("\n")
        o.write ("== Heap ==\n");
        o.write ("\n")
        o.write ("allocation site: resolved type\n")
        o.write ("         observed memory address + alloced bytes\n")
        for key,items in sorted(self.heap.items()):
            o.write ("%08x: %s\n" % (key, type_to_str(namesof(items))))
            for addr,size in self.heap_address[key]:
                o.write ("%8s %08x + %08x\n" % ('', addr, size))
        o.write ("\n")
        o.write ("== Stack ==\n")
        o.write ("\n")
        o.write ("function + offset  : resolved type\n")
        o.write ("         observed memory addresses\n")
        for key,items in sorted(self.stack.items()):
            for typeset,address in items.items():
                typenames = namesof(typeset)
                if not typenames:
                    continue
                o.write ("%08x + %08x: %s\n" % (key[0], key[1], type_to_str(typenames)))
                o.write ("%8s %s\n" % ('', ' '.join(['%08x' % x for x in sorted(address)])))
        o.write ("\n")
        o.write ("== Function ==\n")
        o.write ("\n")
        o.write ("function resolved return type (resolved parameter type, ...)\n")
        o.write ("         observed return value (observed parameter value, ...)\n")
        o.write ("         global variable address: resolved type (observed value)\n")
        for key,func in sorted(self.func.items()):

            regnames = dict([(1, 'eax'), (2, 'ebx'), (3, 'ecx'), (4, 'edx')])
            rparams = ['%s: %s' % (regnames[off], type_to_str(namesof(p.typeset)))  for off,p in sorted(func.rparams.items())]
            rvalues = ['%s: %s' % (regnames[off], v_to_str(p.value))  for off,p in sorted(func.rparams.items())]

            sparams = ['%s' % (type_to_str(namesof(p.typeset)))  for off,p in zip(count(0),func.sparams)]
            svalues = ['%s' % (v_to_str(p.value))  for off,p in zip(count(0),func.sparams)]


            if func.rettype:
                o.write("%08x %s (%s)\n" % (key, type_to_str(namesof(func.rettype.typeset)), ', '.join(chain(rparams, sparams))))
                o.write("%8s %s (%s)\n" % ('', v_to_str(func.rettype.value), ', '.join(chain(rvalues, svalues))))
            else:
                o.write("%08x void (%s)\n" % (key, ', '.join(chain(rparams, sparams))))
                o.write("%8s void (%s)\n" % ('', ', '.join(chain(rvalues, svalues))))

            for mem,val in sorted(func.globaldata.items()):
                if mem in self.data:
                    o.write ("%8s %08x: %s (%08x)\n" % ('', mem, type_to_str(namesof(self.data[mem])), val))
                else:
                    o.write ("%8s %08x: UNKNOWN (%08x)\n" % ('', mem, val))
        o.close()
    

    def Import(self, filename):
        f = open(filename, "r")
        for x in f:
            x = x.strip().split()
            if x[0] == 'D':
                key = int(x[1],16)
                self.data[key].update((int(y,16) for y in x[2:])) 

            if x[0] == 'H':
                key = int(x[1],16)
                self.heap[key].update((int(y,16) for y in x[2:]))

            if x[0] == 'HA':
                addr = int(x[1],16)
                size = int(x[2],16)
                self.heap_address[key].add((addr, size))

            if x[0] == 'S':
                key0 = int(x[1],16)
                key1 = int(x[2],16)
                stack = self.stack[(key0,key1)][frozenset((int(y,16) for y in x[3:]))]

            if x[0] == 'SA':
                stack.update((int(y,16) for y in x[1:]))

            if x[0] == 'F':
                key = int(x[1],16)
                func = self.func[key]

            if x[0] == 'FS':
                off = int(x[1],16)
                val = None if x[2] == 'None' else int(x[2],16)
                func.sparams[off].value = val
                func.sparams[off].typeset.update((int(y,16) for y in x[3:]))

            if x[0] == 'FR':
                off = int(x[1],16)
                val = None if x[2] == 'None' else int(x[2],16)
                func.rparams[off].value = val
                func.rparams[off].typeset.update((int(y,16) for y in x[3:]))

            if x[0] == 'FT':
                val = None if x[1] == 'None' else int(x[1],16)
                func.rettype.value = val
                func.rettype.typeset.update((int(y,16) for y in x[2:]))

            if x[0] == 'FG':
                mem = int(x[1],16)
                val = int(x[2],16)
                func.globaldata[mem] = val
        f.close()


    def Export(self, filename):
        o = open(filename, "w")
        for key,items in sorted(self.data.items()):
            if not items:
                continue
            o.write("D %08x %s\n" % (key, " ".join(("%08x" % t for t in items))))

        for key,items in sorted(self.heap.items()):
            if not items:
                continue
            o.write("H %08x %s\n" % (key, " ".join(("%08x" % t for t in items))))
            if key not in self.heap_address:
                continue
            for addr,size in self.heap_address[key]:
                o.write("HA %08x %08x\n" % (addr, size))

        for key,items in sorted(self.stack.items()):
            if not items: 
                continue
            for typeset,addrs in sorted(items.items()):
                if not typeset:
                    continue
                o.write("S %08x %08x %s\n" % (key[0], key[1], " ".join(("%08x" % t for t in typeset))))
                o.write("SA %s\n" % (" ".join(("%08x" % a for a in addrs))))

        for key,func in sorted(self.func.items()):
            o.write("F %08x\n" % key)
            for off,param in zip(count(0),func.sparams):
                o.write("FS %08x %s %s\n" % (off, v_to_str(param.value), t_to_str(param.typeset)))
            for off,param in func.rparams.items():
                o.write("FR %08x %s %s\n" % (off, v_to_str(param.value), t_to_str(param.typeset)))
            if func.rettype:
                o.write("FT %s %s\n" % (v_to_str(func.rettype.value), t_to_str(func.rettype.typeset)))
            for addr,val in func.globaldata.items():
                o.write("FG %08x %08x\n" % (addr, val))
        o.close()


class Analysis:
    def __init__(self, dirname):
        self.dirname = dirname
        self.types = defaultdict(Type)  # tag -> Type
        self.data  = defaultdict(set)   # addr -> set of tag
        self.stack = defaultdict(set)   # (d1, d2) -> set of (addr, tag)
        self.heap  = defaultdict(set)   # d1 -> set of (addr, tag, d2)
        self.func  = defaultdict(Func)  # addr -> Func

    def do(self):
        out = AnalysisResult()
        f = open("%s/alltrace.out" % self.dirname, errors='ignore')
        for x in f:
            if not (x[0] in "MXCDRTG" and x[1] == ' '):
                continue

            d = x.split()
            if d[0] == 'X' and len(x) == 22:
                tag = int(d[1],16)
                typ = int(d[2],16)
                distance = int(d[3])
                self.types[tag].addtype(typ, distance)

            if d[0] == 'M' and len(x) == 40:
                tag  = int(d[1],16)
                addr = int(d[2],16)
                mtype = int(d[3])
                desc1 = int(d[4],16)
                desc2 = int(d[5],16)

                if mtype == 1:
                    # data section
                    self.data[addr].add(tag)

                if mtype == 2:
                    # heap
                    self.heap[desc1].add((addr, tag, desc2))

                if mtype == 3:
                    # stack
                    self.stack[(desc1, desc2)].add((addr, tag))

            if d[0] == 'C' and len(x) == 47:
                fun = int(d[1],16)
                off = int(d[2],16)
                tag = int(d[3],16)
                val = int(d[4],16)

                #function stack parameter
                self.func[fun].add_sparam(off, tag, val)

            if d[0] == 'D' and len(x) == 47:
                fun = int(d[1],16)
                off = int(d[2],16)
                tag = int(d[3],16)
                val = int(d[4],16)

                #function register parameter
                self.func[fun].add_rparam(off, tag, val)


            if d[0] == 'R' and len(x) == 38:
                fun = int(d[1],16)
                tag = int(d[2],16)
                val = int(d[3],16)

                #function return type
                self.func[fun].add_return(tag, val)

            if d[0] == 'G' and len(x) == 29:
                fun = int(d[1],16)
                mem = int(d[2],16)
                val = int(d[3],16)

                #function global data
                self.func[fun].add_globaldata(mem, val)

            if d[0] == 'T' and len(x) == 560:
                fun = int(d[1],16)

                params = [(int(d[x],16),int(d[x+1],16)) for x in range(3,len(d),2)]
                #function stack image at entrypoint (helps further analysis on stack parameter)
                self.func[fun].add_stackimage(params)


        # Data section
        for key,items in self.data.items():
            out.data[key] = set().union(
                *[self.types[tag].typeset for tag in items])
           
        # Heap
        for key,items in self.heap.items():
            out.heap[key] = set().union(
                *[self.types[tag].typeset for addr,tag,byte in items])

            addrset = dict()
            for addrkey,addr,size in sorted([(addr+byte, addr, byte) for addr,tag,byte in items]):
                if addrkey in addrset:
                    continue
                addrset[addrkey] = (addr, byte)
            out.heap_address[key] = set(addrset.values())

        # Stack
        for key,items in self.stack.items():
            out.stack[key] = stack = defaultdict(set)    # typeset -> set of addr
            for addr, tag in items:
                if not self.types[tag].typeset:
                    continue
                stack[frozenset(self.types[tag].typeset)].add (addr)

        # Function
        for key,func in self.func.items():
            out.func[key] = out_func = FuncResult()

            for off,tag,val in func.rparams:
                p = out_func.rparams[off]
                p.typeset.update(self.types[tag].typeset)
                if p.value is None:
                    p.value = val

            for off,tag,val in func.sparams:
                p = out_func.sparams[off]
                p.typeset.update(self.types[tag].typeset)
                if p.value is None:
                    p.value = val

            for off,p in zip(count(0),out_func.sparams):
                for stack in func.stackimage:
                    if p.typeset:
                        break
                    if off < len(stack):
                        tag = stack[off][0]
                        val = stack[off][1]
                        p.typeset.update(self.types[tag].typeset)
                        if p.value is None:
                            p.value = val

            for tag,val in func.rettype:
                p = out_func.rettype
                p.typeset.update(self.types[tag].typeset)
                if p.value is None:
                    p.value = val

            for addr,val in func.globaldata:
                if addr not in out_func.globaldata:
                    out_func.globaldata[addr] = val

        return out

def main():
    r = AnalysisResult()
    if os.path.exists("int.out"):
        r.Import("int.out")
        
    if len(sys.argv) > 1:
        for path in sys.argv[1:]:
            r.merge(Analysis(path).do())
        r.Export("int.out") 
    else:
        r.PrettyPrint("analysis.out")


typename = [
"UNKNOWN",
"CONFLICT",
"8bit",
"16bit",
"32bit",
"64bit",
"alloc ptr*",
"alloc target",
"void (*f)()",
"esp_t",
"ebp_t",
"ret_addr_t",
"stack_ptr",
"HANDLE",
"LPCHARSETINFO",
"LPDRAWTEXTPARAMS",
"LPFINDREPLACEA",
"LPFINDREPLACEW",
"LPSHELLFLAGSTATE",
"NDR_SCONTEXT",
"NTSTATUS",
"PARRAY_INFO",
"SOCKET",
"WCHAR",
"_Bool",
"_Decimal128",
"_Decimal32",
"_Decimal64",
"__float128",
"char",
"complex double",
"complex float",
"complex long double",
"double",
"float",
"int",
"long double",
"long int",
"long long int",
"long long unsigned int",
"long unsigned int",
"short int",
"short unsigned int",
"signed char",
"unsigned char",
"unsigned int",
"void",
"wchar_t",
"enum ASSOCF",
"enum ASSOCKEY",
"enum ASSOCSTR",
"enum DEVICE_REGISTRY_PROPERTY",
"enum NLS_FUNCTION",
"enum POWER_ACTION",
"enum SHREGDEL_FLAGS",
"enum SHREGENUM_FLAGS",
"enum URLIS",
"enum XLAT_SIDE",
"enum _ACL_INFORMATION_CLASS",
"enum _ATOM_INFORMATION_CLASS",
"enum _AUDIT_EVENT_TYPE",
"enum _BUS_DATA_TYPE",
"enum _CONFIGURATION_TYPE",
"enum _CREATE_FILE_TYPE",
"enum _DEBUG_CONTROL_CODE",
"enum _DEVICE_POWER_STATE",
"enum _DEVICE_RELATION_TYPE",
"enum _DMA_SPEED",
"enum _DMA_WIDTH",
"enum _EVENT_INFORMATION_CLASS",
"enum _EVENT_TYPE",
"enum _EX_POOL_PRIORITY",
"enum _FILE_INFORMATION_CLASS",
"enum _FINDEX_INFO_LEVELS",
"enum _FINDEX_SEARCH_OPS",
"enum _FSINFOCLASS",
"enum _FS_INFORMATION_CLASS",
"enum _GET_FILEEX_INFO_LEVELS",
"enum _HARDERROR_RESPONSE",
"enum _HARDERROR_RESPONSE_OPTION",
"enum _INTERFACE_TYPE",
"enum _INTERLOCKED_RESULT",
"enum _IO_COMPLETION_INFORMATION_CLASS",
"enum _IO_NOTIFICATION_EVENT_CATEGORY",
"enum _JOBOBJECTINFOCLASS",
"enum _KDPC_IMPORTANCE",
"enum _KEY_INFORMATION_CLASS",
"enum _KEY_SET_INFORMATION_CLASS",
"enum _KEY_VALUE_INFORMATION_CLASS",
"enum _KINTERRUPT_MODE",
"enum _KPROFILE_SOURCE",
"enum _KWAIT_REASON",
"enum _LATENCY_TIME",
"enum _LOCK_OPERATION",
"enum _MEMORY_CACHING_TYPE",
"enum _MEMORY_INFORMATION_CLASS",
"enum _MMFLUSH_TYPE",
"enum _MM_PAGE_PRIORITY",
"enum _MM_SYSTEM_SIZE",
"enum _MUTANT_INFORMATION_CLASS",
"enum _OBJECT_INFORMATION_CLASS",
"enum _OBJECT_WAIT_TYPE",
"enum _PARTITION_STYLE",
"enum _POLICY_DOMAIN_INFORMATION_CLASS",
"enum _POLICY_INFORMATION_CLASS",
"enum _POLICY_LOCAL_INFORMATION_CLASS",
"enum _POOL_TYPE",
"enum _PORT_INFORMATION_CLASS",
"enum _POWER_INFORMATION_LEVEL",
"enum _POWER_STATE_TYPE",
"enum _PROCESSINFOCLASS",
"enum _SC_ENUM_TYPE",
"enum _SC_STATUS_TYPE",
"enum _SECTION_INFORMATION_CLASS",
"enum _SECTION_INHERIT",
"enum _SECURITY_IMPERSONATION_LEVEL",
"enum _SECURITY_LOGON_TYPE",
"enum _SEMAPHORE_INFORMATION_CLASS",
"enum _SHUTDOWN_ACTION",
"enum _SID_NAME_USE",
"enum _SOCKADDR_ADDRESS_INFO",
"enum _SOCKADDR_ENDPOINT_INFO",
"enum _SUITE_TYPE",
"enum _SYSDBG_COMMAND",
"enum _SYSTEM_INFORMATION_CLASS",
"enum _SYSTEM_POWER_STATE",
"enum _THREADINFOCLASS",
"enum _TIMER_INFORMATION_CLASS",
"enum _TIMER_TYPE",
"enum _TOKEN_INFORMATION_CLASS",
"enum _TOKEN_TYPE",
"enum _TRACE_INFORMATION_CLASS",
"enum _TRUSTED_INFORMATION_CLASS",
"enum _WORK_QUEUE_TYPE",
"enum _WSACOMPLETIONTYPE",
"enum _WSAESETSERVICEOP",
"enum _WSAEcomparator",
"enum tagCALLCONV",
"enum tagREGKIND",
"enum tagSYSKIND",
"enum tagTOKEN_TYPE",
"union _CLIENT_CALL_RETURN",
"union _FILE_SEGMENT_ELEMENT",
"union _LARGE_INTEGER",
"union _POWER_STATE",
"union _SLIST_HEADER",
"union _ULARGE_INTEGER",
"union tagCY",
"struct __STRUCT0__",
"struct DLGTEMPLATE",
"struct FILE",
"struct HACCEL__",
"struct HBITMAP__",
"struct HBRUSH__",
"struct HCOLORSPACE__",
"struct HCONVLIST__",
"struct HCONV__",
"struct HDC__",
"struct HDDEDATA__",
"struct HDESK__",
"struct HDROP__",
"struct HDRVR__",
"struct HDWP__",
"struct HENHMETAFILE__",
"struct HFONT__",
"struct HGLRC__",
"struct HICON__",
"struct HINSTANCE__",
"struct HKEY__",
"struct HKL__",
"struct HMENU__",
"struct HMETAFILE__",
"struct HMIDIIN__",
"struct HMIDIOUT__",
"struct HMIDISTRM__",
"struct HMIDI__",
"struct HMIXEROBJ__",
"struct HMIXER__",
"struct HMMIO__",
"struct HMONITOR__",
"struct HPALETTE__",
"struct HPEN__",
"struct HPROPSHEETPAGE__",
"struct HRGN__",
"struct HRSRC__",
"struct HSZ__",
"struct HTASK__",
"struct HWAVEIN__",
"struct HWAVEOUT__",
"struct HWINSTA__",
"struct HWND__",
"struct IAdviseSink",
"struct IAdviseSink2",
"struct IAdviseSink2Vtbl",
"struct IAdviseSinkVtbl",
"struct IBindCtx",
"struct IBindCtxVtbl",
"struct IClassFactory",
"struct IClassFactoryVtbl",
"struct ICreateErrorInfo",
"struct ICreateErrorInfoVtbl",
"struct ICreateTypeLib",
"struct IDataAdviseHolder",
"struct IDataAdviseHolderVtbl",
"struct IDataObject",
"struct IDataObjectVtbl",
"struct IDispatch",
"struct IDispatchVtbl",
"struct IDropSource",
"struct IDropSourceVtbl",
"struct IDropTarget",
"struct IDropTargetVtbl",
"struct IEnumFORMATETC",
"struct IEnumFORMATETCVtbl",
"struct IEnumMoniker",
"struct IEnumMonikerVtbl",
"struct IEnumOLEVERB",
"struct IEnumOLEVERBVtbl",
"struct IEnumSTATDATA",
"struct IEnumSTATDATAVtbl",
"struct IEnumSTATSTG",
"struct IEnumSTATSTGVtbl",
"struct IEnumString",
"struct IEnumStringVtbl",
"struct IEnumUnknown",
"struct IEnumUnknownVtbl",
"struct IErrorInfo",
"struct IErrorInfoVtbl",
"struct IExternalConnection",
"struct IExternalConnectionVtbl",
"struct ILockBytes",
"struct ILockBytesVtbl",
"struct IMalloc",
"struct IMallocSpy",
"struct IMallocSpyVtbl",
"struct IMallocVtbl",
"struct IMarshal",
"struct IMarshalVtbl",
"struct IMessageFilter",
"struct IMessageFilterVtbl",
"struct IMoniker",
"struct IMonikerVtbl",
"struct IOleAdviseHolder",
"struct IOleAdviseHolderVtbl",
"struct IOleClientSite",
"struct IOleClientSiteVtbl",
"struct IOleInPlaceActiveObject",
"struct IOleInPlaceActiveObjectVtbl",
"struct IOleInPlaceFrame",
"struct IOleInPlaceFrameVtbl",
"struct IOleObject",
"struct IOleObjectVtbl",
"struct IPSFactoryBuffer",
"struct IPSFactoryBufferVtbl",
"struct IPersist",
"struct IPersistFile",
"struct IPersistFileVtbl",
"struct IPersistStorage",
"struct IPersistStorageVtbl",
"struct IPersistStream",
"struct IPersistStreamVtbl",
"struct IPersistVtbl",
"struct IROTData",
"struct IROTDataVtbl",
"struct IRecordInfo",
"struct IRecordInfoVtbl",
"struct IRootStorage",
"struct IRootStorageVtbl",
"struct IRpcChannelBuffer",
"struct IRpcChannelBufferVtbl",
"struct IRpcProxyBuffer",
"struct IRpcProxyBufferVtbl",
"struct IRpcStubBuffer",
"struct IRpcStubBufferVtbl",
"struct IRunnableObject",
"struct IRunnableObjectVtbl",
"struct IRunningObjectTable",
"struct IRunningObjectTableVtbl",
"struct IShellFolder",
"struct IShellFolderVtbl",
"struct IStdMarshalInfo",
"struct IStdMarshalInfoVtbl",
"struct IStorage",
"struct IStorageVtbl",
"struct IStream",
"struct IStreamVtbl",
"struct ITypeInfo",
"struct ITypeInfoVtbl",
"struct ITypeLib",
"struct ITypeLibVtbl",
"struct IUnknown",
"struct IUnknownVtbl",
"struct MSGBOXPARAMSA",
"struct MSGBOXPARAMSW",
"struct NUMPARSE",
"struct RPC_IF_ID_VECTOR",
"struct RPC_STATS_VECTOR",
"struct SC_HANDLE__",
"struct UDATE",
"struct WSAData",
"struct _ABC",
"struct _ABCFLOAT",
"struct _ACCESS_STATE",
"struct _ACL",
"struct _ADAPTER_OBJECT",
"struct _AFPROTOCOLS",
"struct _AUTH_IDENTITY",
"struct _AppBarData",
"struct _BATTERY_MINIPORT_INFO",
"struct _BLOB",
"struct _BOOTDISK_INFORMATION",
"struct _BY_HANDLE_FILE_INFORMATION",
"struct _CACHE_MANAGER_CALLBACKS",
"struct _CACHE_UNINITIALIZE_EVENT",
"struct _CALLBACK_OBJECT",
"struct _CC_FILE_SIZES",
"struct _CHAR_INFO",
"struct _CLIENT_ID",
"struct _CM_FULL_RESOURCE_DESCRIPTOR",
"struct _CM_PARTIAL_RESOURCE_DESCRIPTOR",
"struct _CM_PARTIAL_RESOURCE_LIST",
"struct _CM_RESOURCE_LIST",
"struct _COAUTHINFO",
"struct _COLORMAP",
"struct _COMMPROP",
"struct _COMMTIMEOUTS",
"struct _COMM_CONFIG",
"struct _COMM_FAULT_OFFSETS",
"struct _COMPRESSED_DATA_INFO",
"struct _COMSTAT",
"struct _CONFIGURATION_INFORMATION",
"struct _CONNECTDLGSTRUCTA",
"struct _CONNECTDLGSTRUCTW",
"struct _CONSOLE_CURSOR_INFO",
"struct _CONSOLE_SCREEN_BUFFER_INFO",
"struct _CONTEXT",
"struct _CONTROLLER_OBJECT",
"struct _COORD",
"struct _COSERVERINFO",
"struct _CREATE_DISK",
"struct _CRITICAL_SECTION",
"struct _CRITICAL_SECTION_DEBUG",
"struct _CSADDR_INFO",
"struct _DCB",
"struct _DEBUG_BUFFER",
"struct _DEBUG_EVENT",
"struct _DEVICE_DESCRIPTION",
"struct _DEVICE_OBJECT",
"struct _DEVOBJ_EXTENSION",
"struct _DISCDLGSTRUCTA",
"struct _DISCDLGSTRUCTW",
"struct _DISK_SIGNATURE",
"struct _DISPATCHER_HEADER",
"struct _DISPLAY_DEVICEA",
"struct _DISPLAY_DEVICEW",
"struct _DMA_ADAPTER",
"struct _DMA_OPERATIONS",
"struct _DOCINFOA",
"struct _DOCINFOW",
"struct _DPA",
"struct _DRIVER_EXTENSION",
"struct _DRIVER_OBJECT",
"struct _DRIVE_LAYOUT_INFORMATION",
"struct _DRIVE_LAYOUT_INFORMATION_EX",
"struct _DSA",
"struct _ENUM_SERVICE_STATUSA",
"struct _ENUM_SERVICE_STATUSW",
"struct _EPROCESS",
"struct _ERESOURCE",
"struct _ETHREAD",
"struct _EXCEPTION_POINTERS",
"struct _EXCEPTION_RECORD",
"struct _EXCEPTION_REGISTRATION_RECORD",
"struct _FAST_IO_DISPATCH",
"struct _FAST_MUTEX",
"struct _FILETIME",
"struct _FILE_BASIC_INFORMATION",
"struct _FILE_FULL_EA_INFORMATION",
"struct _FILE_LOCK",
"struct _FILE_LOCK_INFO",
"struct _FILE_NETWORK_OPEN_INFORMATION",
"struct _FILE_OBJECT",
"struct _FILE_QUOTA_INFORMATION",
"struct _FIXED",
"struct _FLOATING_SAVE_AREA",
"struct _FULL_PTR_XLAT_TABLES",
"struct _GENERATE_NAME_CONTEXT",
"struct _GENERIC_BINDING_ROUTINE_PAIR",
"struct _GENERIC_MAPPING",
"struct _GLYPHMETRICS",
"struct _GLYPHMETRICSFLOAT",
"struct _GUID",
"struct _ICONINFO",
"struct _IMAGEINFO",
"struct _IMAGELIST",
"struct _INITIAL_TEB",
"struct _INPUT_RECORD",
"struct _IO_COMPLETION_CONTEXT",
"struct _IO_CSQ",
"struct _IO_CSQ_IRP_CONTEXT",
"struct _IO_REMOVE_LOCK",
"struct _IO_REMOVE_LOCK_COMMON_BLOCK",
"struct _IO_RESOURCE_DESCRIPTOR",
"struct _IO_RESOURCE_LIST",
"struct _IO_RESOURCE_REQUIREMENTS_LIST",
"struct _IO_STATUS_BLOCK",
"struct _IO_TIMER",
"struct _IO_WORKITEM",
"struct _IRP",
"struct _ITEMIDLIST",
"struct _KAPC",
"struct _KAPC_STATE",
"struct _KBUGCHECK_CALLBACK_RECORD",
"struct _KDEVICE_QUEUE",
"struct _KDEVICE_QUEUE_ENTRY",
"struct _KDPC",
"struct _KEVENT",
"struct _KEY_MULTIPLE_VALUE_INFORMATION",
"struct _KEY_VALUE_ENTRY",
"struct _KFLOATING_SAVE",
"struct _KINTERRUPT",
"struct _KLOCK_QUEUE_HANDLE",
"struct _KMUTANT",
"struct _KPCR",
"struct _KPCR_TIB",
"struct _KPRCB",
"struct _KPROCESS",
"struct _KQUEUE",
"struct _KSEMAPHORE",
"struct _KSPIN_LOCK_QUEUE",
"struct _KTHREAD",
"struct _KTIMER",
"struct _KTSS",
"struct _KWAIT_BLOCK",
"struct _LDT_ENTRY",
"struct _LIST_ENTRY",
"struct _LPC_MESSAGE",
"struct _LPC_SECTION_MEMORY",
"struct _LPC_SECTION_OWNER_MEMORY",
"struct _LPC_SECTION_READ",
"struct _LPC_SECTION_WRITE",
"struct _LSA_AUTH_INFORMATION",
"struct _LSA_OBJECT_ATTRIBUTES",
"struct _LSA_REFERENCED_DOMAIN_LIST",
"struct _LSA_TRANSLATED_NAME",
"struct _LSA_TRANSLATED_SID",
"struct _LSA_TRUST_INFORMATION",
"struct _LUID",
"struct _LUID_AND_ATTRIBUTES",
"struct _MALLOC_FREE_STRUCT",
"struct _MAT2",
"struct _MDL",
"struct _MEMORYSTATUS",
"struct _MEMORY_BASIC_INFORMATION",
"struct _MIDL_STUB_DESC",
"struct _MIDL_STUB_MESSAGE",
"struct _MMCKINFO",
"struct _MMIOINFO",
"struct _NCB",
"struct _NDR_CS_ROUTINES",
"struct _NDR_CS_SIZE_CONVERT_ROUTINES",
"struct _NETCONNECTINFOSTRUCT",
"struct _NETINFOSTRUCT",
"struct _NETRESOURCEA",
"struct _NETRESOURCEW",
"struct _NOTIFYICONDATAA",
"struct _NOTIFYICONDATAW",
"struct _NOTIFY_SYNC",
"struct _NPAGED_LOOKASIDE_LIST",
"struct _NT_TIB",
"struct _OBJDIR_INFORMATION",
"struct _OBJECT_ATTRIBUTES",
"struct _OBJECT_HANDLE_INFORMATION",
"struct _OBJECT_NAME_INFORMATION",
"struct _OBJECT_TYPE",
"struct _OFSTRUCT",
"struct _OLESTREAM",
"struct _OLESTREAMVTBL",
"struct _OSVERSIONINFOA",
"struct _OSVERSIONINFOEXA",
"struct _OSVERSIONINFOEXW",
"struct _OSVERSIONINFOW",
"struct _OUTLINETEXTMETRICA",
"struct _OUTLINETEXTMETRICW",
"struct _OVERLAPPED",
"struct _OWNER_ENTRY",
"struct _PAGED_LOOKASIDE_LIST",
"struct _PHYSICAL_MEMORY_RANGE",
"struct _POINTFLOAT",
"struct _POLYTEXTA",
"struct _POLYTEXTW",
"struct _PRINTER_DEFAULTSA",
"struct _PRINTER_DEFAULTSW",
"struct _PRINTER_NOTIFY_INFO",
"struct _PRINTER_NOTIFY_INFO_DATA",
"struct _PRIVILEGE_SET",
"struct _PROCESS_HEAP_ENTRY",
"struct _PROCESS_INFORMATION",
"struct _PROPSHEETHEADERA",
"struct _PROPSHEETHEADERW",
"struct _PROPSHEETPAGEA",
"struct _PROPSHEETPAGEW",
"struct _QUERY_SERVICE_CONFIGA",
"struct _QUERY_SERVICE_CONFIGW",
"struct _QUERY_SERVICE_LOCK_STATUSA",
"struct _QUERY_SERVICE_LOCK_STATUSW",
"struct _QUOTA_LIMITS",
"struct _QualityOfService",
"struct _RANGE_LIST_ITERATOR",
"struct _RASTERIZER_STATUS",
"struct _REPARSE_DATA_BUFFER",
"struct _RGNDATA",
"struct _RGNDATAHEADER",
"struct _RPC_BINDING_VECTOR",
"struct _RPC_IF_ID",
"struct _RPC_MESSAGE",
"struct _RPC_POLICY",
"struct _RPC_PROTSEQ_VECTORA",
"struct _RPC_PROTSEQ_VECTORW",
"struct _RPC_SECURITY_QOS",
"struct _RPC_SYNTAX_IDENTIFIER",
"struct _RPC_TRANSFER_SYNTAX",
"struct _RPC_VERSION",
"struct _RTL_BITMAP",
"struct _RTL_BITMAP_RUN",
"struct _RTL_OSVERSIONINFOEXW",
"struct _RTL_OSVERSIONINFOW",
"struct _RTL_QUERY_REGISTRY_TABLE",
"struct _RTL_RANGE",
"struct _RTL_RANGE_LIST",
"struct _RTL_SPLAY_LINKS",
"struct _RTL_USER_PROCESS_PARAMETERS",
"struct _SECTION_OBJECT",
"struct _SECTION_OBJECT_POINTERS",
"struct _SECURITY_ATTRIBUTES",
"struct _SECURITY_CLIENT_CONTEXT",
"struct _SECURITY_DESCRIPTOR",
"struct _SECURITY_QUALITY_OF_SERVICE",
"struct _SECURITY_SUBJECT_CONTEXT",
"struct _SERVICE_STATUS",
"struct _SERVICE_TABLE_ENTRYA",
"struct _SERVICE_TABLE_ENTRYW",
"struct _SET_PARTITION_INFORMATION_EX",
"struct _SHARE_ACCESS",
"struct _SHELLEXECUTEINFOA",
"struct _SHELLEXECUTEINFOW",
"struct _SHFILEINFOA",
"struct _SHFILEINFOW",
"struct _SHFILEOPSTRUCTA",
"struct _SHFILEOPSTRUCTW",
"struct _SHITEMID",
"struct _SHQUERYRBINFO",
"struct _SID",
"struct _SID_AND_ATTRIBUTES",
"struct _SID_IDENTIFIER_AUTHORITY",
"struct _SINGLE_LIST_ENTRY",
"struct _SMALL_RECT",
"struct _SOCKADDR_INFO",
"struct _SOCKET_ADDRESS",
"struct _STARTUPINFOA",
"struct _STARTUPINFOW",
"struct _STRING",
"struct _STRRET",
"struct _SYSTEMTIME",
"struct _SYSTEM_INFO",
"struct _SYSTEM_POWER_STATUS",
"struct _TBBUTTON",
"struct _TEB",
"struct _TIME_FIELDS",
"struct _TIME_ZONE_INFORMATION",
"struct _TOKEN_CONTROL",
"struct _TOKEN_DEFAULT_DACL",
"struct _TOKEN_GROUPS",
"struct _TOKEN_OWNER",
"struct _TOKEN_PRIMARY_GROUP",
"struct _TOKEN_PRIVILEGES",
"struct _TOKEN_SOURCE",
"struct _TOKEN_USER",
"struct _TRUSTED_DOMAIN_AUTH_INFORMATION",
"struct _TRUSTED_DOMAIN_INFORMATION_EX",
"struct _TUNNEL",
"struct _UNICODE_STRING",
"struct _USER_MARSHAL_ROUTINE_QUADRUPLE",
"struct _USER_STACK",
"struct _UUID_VECTOR",
"struct _VPB",
"struct _WIN32_FIND_DATAA",
"struct _WIN32_FIND_DATAW",
"struct _WINDOWPLACEMENT",
"struct _WINSOCK_MAPPING",
"struct _WNDCLASSA",
"struct _WNDCLASSEXA",
"struct _WNDCLASSEXW",
"struct _WNDCLASSW",
"struct _WORK_QUEUE_ITEM",
"struct _WSABUF",
"struct _WSACOMPLETION",
"struct _WSANAMESPACE_INFOA",
"struct _WSANAMESPACE_INFOW",
"struct _WSANETWORKEVENTS",
"struct _WSANSClassInfoA",
"struct _WSANSClassInfoW",
"struct _WSAPROTOCOLCHAIN",
"struct _WSAPROTOCOL_INFOA",
"struct _WSAPROTOCOL_INFOW",
"struct _WSAQuerySetA",
"struct _WSAQuerySetW",
"struct _WSAServiceClassInfoA",
"struct _WSAServiceClassInfoW",
"struct _WSAVersion",
"struct _XFORM",
"struct _XMIT_ROUTINE_QUINTUPLE",
"struct _ZONE_HEADER",
"struct _browseinfoA",
"struct _browseinfoW",
"struct _cpinfo",
"struct _cpinfoexA",
"struct _cpinfoexW",
"struct _currencyfmtA",
"struct _currencyfmtW",
"struct _devicemodeA",
"struct _devicemodeW",
"struct _finddata_t",
"struct _finddatai64_t",
"struct _flowspec",
"struct _numberfmtA",
"struct _numberfmtW",
"struct _stat",
"struct _stati64",
"struct _wfinddata_t",
"struct _wfinddatai64_t",
"struct div_t",
"struct fd_set",
"struct hostent",
"struct in_addr",
"struct joyinfo_tag",
"struct joyinfoex_tag",
"struct ldiv_t",
"struct lldiv_t",
"struct midihdr_tag",
"struct mmtime_tag",
"struct nlsversioninfo",
"struct protoent",
"struct servent",
"struct sockaddr",
"struct stat",
"struct tMIXERCONTROLDETAILS",
"struct tWAVEFORMATEX",
"struct tagACCEL",
"struct tagALTTABINFO",
"struct tagAUXCAPSA",
"struct tagAUXCAPSW",
"struct tagBIND_OPTS",
"struct tagBITMAP",
"struct tagBITMAPINFO",
"struct tagBITMAPINFOHEADER",
"struct tagCANDIDATEFORM",
"struct tagCANDIDATELIST",
"struct tagCHOOSECOLORA",
"struct tagCHOOSECOLORW",
"struct tagCHOOSEFONTA",
"struct tagCHOOSEFONTW",
"struct tagCIEXYZ",
"struct tagCIEXYZTRIPLE",
"struct tagCOLORADJUSTMENT",
"struct tagCOMBOBOXINFO",
"struct tagCOMPOSITIONFORM",
"struct tagCONVCONTEXT",
"struct tagCONVINFO",
"struct tagCURSORINFO",
"struct tagDEC",
"struct tagDISPPARAMS",
"struct tagDVTARGETDEVICE",
"struct tagENHMETAHEADER",
"struct tagENHMETARECORD",
"struct tagEXCEPINFO",
"struct tagFONTSIGNATURE",
"struct tagFORMATETC",
"struct tagGCP_RESULTSA",
"struct tagGCP_RESULTSW",
"struct tagHANDLETABLE",
"struct tagHW_PROFILE_INFOA",
"struct tagHW_PROFILE_INFOW",
"struct tagIMEMENUITEMINFOA",
"struct tagIMEMENUITEMINFOW",
"struct tagINTERFACEDATA",
"struct tagINTERFACEINFO",
"struct tagJOYCAPSA",
"struct tagJOYCAPSW",
"struct tagKERNINGPAIR",
"struct tagLASTINPUTINFO",
"struct tagLAYERPLANEDESCRIPTOR",
"struct tagLOGBRUSH",
"struct tagLOGCOLORSPACEA",
"struct tagLOGCOLORSPACEW",
"struct tagLOGFONTA",
"struct tagLOGFONTW",
"struct tagLOGPALETTE",
"struct tagLOGPEN",
"struct tagMENUBARINFO",
"struct tagMENUINFO",
"struct tagMENUITEMINFOA",
"struct tagMENUITEMINFOW",
"struct tagMETAFILEPICT",
"struct tagMETARECORD",
"struct tagMETHODDATA",
"struct tagMIDIINCAPSA",
"struct tagMIDIINCAPSW",
"struct tagMIDIOUTCAPSA",
"struct tagMIDIOUTCAPSW",
"struct tagMIXERCAPSA",
"struct tagMIXERCAPSW",
"struct tagMIXERCONTROLA",
"struct tagMIXERCONTROLW",
"struct tagMIXERLINEA",
"struct tagMIXERLINECONTROLSA",
"struct tagMIXERLINECONTROLSW",
"struct tagMIXERLINEW",
"struct tagMONITORINFO",
"struct tagMSG",
"struct tagMULTI_QI",
"struct tagOFNA",
"struct tagOFNW",
"struct tagOIFI",
"struct tagOleMenuGroupWidths",
"struct tagPAINTSTRUCT",
"struct tagPALETTEENTRY",
"struct tagPANOSE",
"struct tagPARAMDATA",
"struct tagPDA",
"struct tagPDW",
"struct tagPIXELFORMATDESCRIPTOR",
"struct tagPOINT",
"struct tagPSDA",
"struct tagPSDW",
"struct tagRECT",
"struct tagRECTL",
"struct tagRGBQUAD",
"struct tagRPCOLEMESSAGE",
"struct tagRemSNB",
"struct tagRemSTGMEDIUM",
"struct tagSAFEARRAY",
"struct tagSAFEARRAYBOUND",
"struct tagSCROLLBARINFO",
"struct tagSCROLLINFO",
"struct tagSIZE",
"struct tagSOLE_AUTHENTICATION_SERVICE",
"struct tagSTATDATA",
"struct tagSTATSTG",
"struct tagSTGMEDIUM",
"struct tagSTGOPTIONS",
"struct tagSTYLEBUFA",
"struct tagSTYLEBUFW",
"struct tagTEXTMETRICA",
"struct tagTEXTMETRICW",
"struct tagTPMPARAMS",
"struct tagTRACKMOUSEEVENT",
"struct tagVARIANT",
"struct tagWAVEINCAPSA",
"struct tagWAVEINCAPSW",
"struct tagWAVEOUTCAPSA",
"struct tagWAVEOUTCAPSW",
"struct tagWINDOWINFO",
"struct timecaps_tag",
"struct timeval",
"struct tm",
"struct value_entA",
"struct value_entW",
"struct wavehdr_tag"
]


def namesof(tlist):
#    if len(tlist) == 1:
#        return nameof(tlist[0])
#    else:
#        return '[' + ', '.join([nameof(t) for t in tlist]) + ']'
    s = set([nameof(t) for t in tlist])
    s.discard("UNKNOWN")
    return s


def nameof(t):
    global typename
    ta = t & TYPE_BIT_MASK
    if ta < len(typename):
        return '%s%s%s' % (typename[ta],
                           '*' if t & POINTER_BIT else '',
                           '**' if t & DBL_POINTER_BIT else '')
    return 'UNKNOWN'


def type_to_str(s):
    assert (isinstance(s, set) or isinstance(s, frozenset) or s is None)

    if s is None or len(s) == 0:
        return 'UNKNOWN'

    s = set(s)

    if '8bit' in s or '16bit' in s or '32bit' in s or '64bit' in s:
        for x in s:
            if not x.endswith('bit'):
                s.discard('8bit')
                s.discard('16bit')
                s.discard('32bit')
                s.discard('64bit')
                break

    if '8bit*' in s or '16bit*' in s or '32bit*' in s or '64bit*' in s:
        for x in s:
            if (x.endswith('*') and not x.endswith('bit*')) or x == 'void (*f)()':
                s.discard('8bit*')
                s.discard('16bit*')
                s.discard('32bit*')
                s.discard('64bit*')
                break


    if 'long unsigned int' in s:
        s.discard('long unsigned int')
        s.add('unsigned int')

    if 'long unsigned int*' in s:
        s.discard('long unsigned int*')
        s.add('unsigned int*')

    if 'long int' in s:
        s.discard('long int')
        s.add('int')

    if 'long int*' in s:
        s.discard('long int*')
        s.add('int*')

    if 'int' in s and 'unsigned int' in s:
        s.discard('unsigned int')

    if 'int*' in s and 'unsigned int*' in s:
        s.discard('unsigned int*')

    if 'char*' in s and '8bit*' in s:
        s.discard('8bit*')

    if 'int*' in s and '32bit*' in s:
        s.discard('32bit*')

    if 'NTSTATUS' in s or 'HANDLE' in s:
        s.discard('int')
        s.discard('unsigned int')

    if 'SOCKET' in s:
        s.discard('int')
        s.discard('unsigned int')
        s.discard('HANDLE')


    hasPtr = False
    hasPtrNotVoid = False

    for x in s:
        if x.endswith('*') or x == 'void (*f)()':
            hasPtr = True
            if x != 'void*':
                hasPtrNotVoid = True

    if hasPtrNotVoid:
        s.discard("void*")
    if hasPtr:
        s.discard("int")

    if len(s) == 0:
        return 'UNKNOWN'
    if len(s) == 1:
        return s.pop()
    return '[%s]' % (', '.join(sorted(s)))



if __name__ == '__main__':
    main() 
