SRCS = di-opt.cpp

all: di-opt

ECHO=echo
CXX=g++ #clang++ -fsanitize=undefined
CFLAGS += -I../include
CFLAGS += -fasynchronous-unwind-tables
CFLAGS += -fPIC
CFLAGS += -g
CFLAGS += -D__STDC_LIMIT_MACROS
CFLAGS += -Wno-deprecated
CFLAGS += -Dos_linux
CFLAGS += -Darch_x86_64
CFLAGS += -Dx86_64_unknown_linux2_4
CFLAGS += -DARCH=x64
CFLAGS += -std=c++11
CFLAGS += -rdynamic

INSTALLDIR=../bin

# Dyninst headers
include ../Makefile.inc

INCLUDES += -I$(DYNINST_ROOT)/common/h
INCLUDES += -I$(DYNINST_ROOT)/dataflowAPI/h
INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI/h
INCLUDES += -I$(DYNINST_ROOT)/elf/h
INCLUDES += -I$(DYNINST_ROOT)/instructionAPI/h
INCLUDES += -I$(DYNINST_ROOT)/parseAPI/h
INCLUDES += -I$(DYNINST_ROOT)/patchAPI/h
INCLUDES += -I$(DYNINST_ROOT)/proccontrol/h
INCLUDES += -I$(DYNINST_ROOT)/symtabAPI/h
INCLUDES += -I$(DYNINST_ROOT)/stackwalk/h

LDFLAGS += -ldl -lrt
LDFLAGS += -rdynamic
#LDFLAGS += -Wl,-undefined,error
#LDFLAGS += -Wl,--export-dynamic
LDFLAGS += -L$(DYNINST_ROOT)/dynC_API -ldynC_API
LDFLAGS += -L$(DYNINST_ROOT)/common -lcommon
LDFLAGS += -L$(DYNINST_ROOT)/dyninstAPI -ldyninstAPI
LDFLAGS += -L$(DYNINST_ROOT)/instructionAPI -linstructionAPI
LDFLAGS += -L$(DYNINST_ROOT)/parseAPI -lparseAPI
LDFLAGS += -L$(DYNINST_ROOT)/patchAPI -lpatchAPI
LDFLAGS += -L$(DYNINST_ROOT)/symtabAPI -lsymtabAPI

LDFLAGS += -L$(DYNAMORIO_ROOT)/lib64/release -ldynamorio

INSTALL_DIR = ../bin

all: di-opt

di-opt: di-opt.o
	$(QUIET) $(ECHO) "  [LINK] $@"
	$(QUIET) $(CXX) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LIBS)

%.o: %.cpp $(HEADERS)
	$(QUIET) $(ECHO)  "  [C++] $<"
	$(QUIET) $(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

install: $(INSTALL_DIR)/di-opt

$(INSTALL_DIR)/di-opt: di-opt
	install -c -D -m 744 $? $@

clean:
	rm -f *.o di-opt
