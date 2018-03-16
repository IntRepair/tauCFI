
INSTALL_DIR = ../bin

PASSNAME := padyn

OBJS	:= padyn.o

OBJS	+= ast/parsing/ast_creator.o
OBJS	+= ast/parsing/address_taken.o
OBJS	+= ast/parsing/function_borders.o
OBJS	+= ast/parsing/plt_resolver.o
OBJS	+= ast/parsing/region_data.o

OBJS 	+= ast/ast.o
OBJS 	+= ast/function.o
OBJS 	+= ast/basic_block.o
OBJS 	+= ast/instruction.o

OBJS	+= liveness/liveness_analysis.o
OBJS	+= liveness/liveness_count.o
OBJS	+= liveness/liveness_count_ext.o
OBJS	+= liveness/liveness_type.o

OBJS	+= reaching/reaching_analysis.o
OBJS	+= reaching/reaching_count.o
OBJS	+= reaching/reaching_count_ext.o
OBJS	+= reaching/reaching_type.o

OBJS	+= callsites.o
OBJS	+= calltargets.o
OBJS	+= ca_decoder.o
OBJS	+= verification.o
OBJS	+= register_states.o

#OBJS	+= patching.o

INCLUDES += -I`pwd`

# Dyninst headers
include ../Makefile.inc
INCLUDES += -I$(DYNINST_ROOT)/common/h
INCLUDES += -I$(DYNINST_ROOT)/dataflowAPI/h
INCLUDES += -I$(DYNINST_ROOT)/dynC_API/h
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
#CFLAGS += -rdynamic
CFLAGS += -O3
CFLAGS += -Wall

# comment out for type base policy
 CFLAGS += -D__PADYN_TYPE_POLICY
# comment out for extended count policy
# CFLAGS += -D__PADYN_COUNT_EXT_POLICY

# Make DynamoRIO happy
FFLAGS += -DLINUX
FFLAGS += -DX86_64

#LDFLAGS += -Wl,-undefined,error
#LDFLAGS += -Wl,--export-dynamic
LDFLAGS += -ldl -lrt -rdynamic
LDFLAGS += -L$(DYNINST_ROOT)/common -lcommon
LDFLAGS += -L$(DYNINST_ROOT)/dynC_API -ldynC_API
LDFLAGS += -L$(DYNINST_ROOT)/dyninstAPI -ldyninstAPI
LDFLAGS += -L$(DYNINST_ROOT)/instructionAPI -linstructionAPI
LDFLAGS += -L$(DYNINST_ROOT)/parseAPI -lparseAPI
LDFLAGS += -L$(DYNINST_ROOT)/patchAPI -lpatchAPI
LDFLAGS += -L$(DYNINST_ROOT)/symtabAPI -lsymtabAPI
LDFLAGS += -L$(DYNAMORIO_ROOT)/lib64/release -ldynamorio


CC=gcc #clang -fsanitize=undefined
CXX=g++ #clang++ -fsanitize=undefined
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
	install -c -D -m 744 $(PASSLIBNAME) $(INSTALL_DIR)
