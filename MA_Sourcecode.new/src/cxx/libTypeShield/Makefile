include Makefile.inc

ECHO := echo

NAME := libTypeShield

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
OBJS	+= liveness/liveness_impl.o
OBJS	+= liveness/liveness_count.o
#OBJS	+= liveness/liveness_count_ext.o
OBJS	+= liveness/liveness_type.o

OBJS	+= reaching/reaching_analysis.o
OBJS	+= reaching/reaching_count.o
#OBJS	+= reaching/reaching_count_ext.o
OBJS	+= reaching/reaching_type.o

OBJS	+= analysis.o
OBJS	+= callsites.o
OBJS	+= calltargets.o
OBJS	+= ca_decoder.o
OBJS	+= verification.o
OBJS	+= register_states.o

#OBJS	+= patching.o

INCLUDES += -I`pwd`

# Use if Dyninst is not installed
#INCLUDES += -I$(DYNINST_ROOT)/common/h
#INCLUDES += -I$(DYNINST_ROOT)/dataflowAPI/h
#INCLUDES += -I$(DYNINST_ROOT)/dynC_API/h
#INCLUDES += -I$(DYNINST_ROOT)/dyninstAPI/h
#INCLUDES += -I$(DYNINST_ROOT)/elf/h
#INCLUDES += -I$(DYNINST_ROOT)/instructionAPI/h
#INCLUDES += -I$(DYNINST_ROOT)/parseAPI/h
#INCLUDES += -I$(DYNINST_ROOT)/patchAPI/h
#INCLUDES += -I$(DYNINST_ROOT)/proccontrol/h
#INCLUDES += -I$(DYNINST_ROOT)/symtabAPI/h

INCLUDES += -I$(DYNAMORIO_ROOT)/include

CFLAGS += -fPIC
CFLAGS += --std=c++11
CFLAGS += -O3
CFLAGS += -Wall

# Make DynamoRIO happy
FFLAGS += -DLINUX
FFLAGS += -DX86_64

LDFLAGS += -ldl -lrt -rdynamic

# Use if Dyninst is not installed
#LDFLAGS += -L$(DYNINST_ROOT)/dynC_API
#LDFLAGS += -L$(DYNINST_ROOT)/common
#LDFLAGS += -L$(DYNINST_ROOT)/dyninstAPI
#LDFLAGS += -L$(DYNINST_ROOT)/instructionAPI
#LDFLAGS += -L$(DYNINST_ROOT)/parseAPI
#LDFLAGS += -L$(DYNINST_ROOT)/patchAPI
#LDFLAGS += -L$(DYNINST_ROOT)/symtabAPI

LDFLAGS += -lcommon
LDFLAGS += -ldyninstAPI
LDFLAGS += -ldynC_API
LDFLAGS += -linstructionAPI
LDFLAGS += -lparseAPI
LDFLAGS += -lpatchAPI
LDFLAGS += -lsymtabAPI

LDFLAGS += -L$(DYNAMORIO_ROOT)/lib64/release -ldynamorio

LIBRARYNAME =$(NAME).so

all: $(LIBRARYNAME) $(EXTRA_TARGETS)

$(LIBRARYNAME): $(OBJS)
	$(QUIET) $(ECHO) "  [LINK] $@"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) -shared -o $@ $(CPPS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(QUIET) $(ECHO)  "  [C++] $<"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	$(QUIET) $(ECHO) "  [RM] $(OBJS) $(LIBRARYNAME) $(CLEANUP)"
	$(QUIET) rm -f $(OBJS) $(LIBRARYNAME) $(CLEANUP)

install:
	install -c -D -m 744 $(LIBRARYNAME) $(BINARY_DIR)
