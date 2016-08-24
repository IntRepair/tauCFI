
INSTALL_DIR = ../bin

PASSNAME := padyn
OBJS     := padyn.o address_taken.o callsites.o calltargets.o ca_decoder.o liveness_analysis.o reaching_analysis.o patching.o simple_type_analysis_callsite.o simple_type_analysis_calltarget.o

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

INCLUDES += -I$(DYNAMORIO_ROOT)/include

# Code definitions
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
CFLAGS += --std=c++11
CFLAGS += -rdynamic

# Make DynamoRIO happy
FFLAGS += -DLINUX
FFLAGS += -DX86_64

LDFLAGS += -L$(DYNAMORIO_ROOT)/lib64/release
LDFLAGS += -ldynamorio

CC=gcc
CXX=g++
ECHO=echo

PASSLIBNAME =$(PASSNAME).di

all: $(PASSLIBNAME) $(EXTRA_TARGETS)

$(PASSLIBNAME): $(OBJS)
	$(QUIET) $(ECHO) "  [LINK] $@"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) -shared -o $@ $(CPPS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(QUIET) $(ECHO)  "  [C++] $<"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	$(QUIET) $(ECHO) "  [RM] $(OBJS) $(PASSLIBNAME) $(CLEANUP)"
	$(QUIET) rm -f $(OBJS) $(PASSLIBNAME) $(CLEANUP)

install:
	install -c -D -m 744 $(PASSNAME).di $(INSTALL_DIR)
