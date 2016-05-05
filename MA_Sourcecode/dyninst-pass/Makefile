
INSTALL_DIR = ../bin

PASSNAME := padyn
OBJS     := padyn.o address_taken.o relative_callsites.o calltargets.o ca_decoder_dynamo_rio.o

# Dyninst headers
include ../Makefile.inc
INCLUDES += -I$(DYNINST_ROOT)/symlite/h
INCLUDES += -I$(DYNINST_ROOT)/common/h
INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI/h
INCLUDES += -I$(DYNINST_ROOT)/instructionAPI/h
INCLUDES += -I$(DYNINST_ROOT)/dataflowAPI/h
INCLUDES += -I$(DYNINST_ROOT)/symtabAPI/h
INCLUDES += -I$(DYNINST_ROOT)/stackwalk/h
INCLUDES += -I$(DYNINST_ROOT)/dynC_API/h
INCLUDES += -I$(DYNINST_ROOT)/dwarf/h
INCLUDES += -I$(DYNINST_ROOT)/proccontrol/h
INCLUDES += -I$(DYNINST_ROOT)/elf/h
INCLUDES += -I$(DYNINST_ROOT)/parseAPI/h
INCLUDES += -I$(DYNINST_ROOT)/patchAPI/h
INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI_RT/h
INCLUDES += -I$(DYNINST_ROOT)/symlite/src
INCLUDES += -I$(DYNINST_ROOT)/common/src
INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI/src
INCLUDES += -I$(DYNINST_ROOT)/instructionAPI/src
INCLUDES += -I$(DYNINST_ROOT)/dataflowAPI/src
INCLUDES += -I$(DYNINST_ROOT)/symtabAPI/src
INCLUDES += -I$(DYNINST_ROOT)/stackwalk/src
INCLUDES += -I$(DYNINST_ROOT)/dynC_API/src
INCLUDES += -I$(DYNINST_ROOT)/dwarf/src
INCLUDES += -I$(DYNINST_ROOT)/proccontrol/src
INCLUDES += -I$(DYNINST_ROOT)/elf/src
INCLUDES += -I$(DYNINST_ROOT)/parseThat/src
INCLUDES += -I$(DYNINST_ROOT)/parseAPI/src
INCLUDES += -I$(DYNINST_ROOT)/patchAPI/src
INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI_RT/src
INCLUDES += -I$(DYNINST_IROOT)/common/h
INCLUDES += -I$(DYNAMORIO_ROOT)/include

# Code definitions
CFLAGS += -fasynchronous-unwind-tables
CFLAGS += -fPIC
CFLAGS += -g
CFLAGS += -I../include
CFLAGS += -D__STDC_LIMIT_MACROS
CFLAGS += -Wno-deprecated
CFLAGS += -Dos_linux
CFLAGS += -I$(DYNINST_ROOT)
CFLAGS += -Darch_x86_64
CFLAGS += -Dx86_64_unknown_linux2_4
CFLAGS += -DARCH=x64
CFLAGS += --std=c++11
CFLAGS += -I ../include/patharmor


FFLAGS += -DARMOR_WRAP_LIBRARIES
FFLAGS += -DARMOR_WRAP_INDIRECTS
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
