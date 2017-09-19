include Makefile.inc

ECHO := echo

NAME := analyzer

OBJS := main.o

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
INCLUDES += -I../libTypeShield

CFLAGS += --std=c++11
CFLAGS += -O3
CFLAGS += -Wall

# Make DynamoRIO happy
FFLAGS += -DLINUX
FFLAGS += -DX86_64

LDFLAGS += -ldl -lrt
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

LDFLAGS += -L../libTypeShield -lTypeShield

LDFLAGS += -L$(DYNAMORIO_ROOT)/lib64/release -ldynamorio

BINARYNAME=$(NAME)

all: $(BINARYNAME)

$(BINARYNAME): $(OBJS)
	$(QUIET) $(ECHO) "  [LINK] $@"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) -o $@ $(CPPS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(QUIET) $(ECHO)  "  [C++] $<"
	$(QUIET) $(CXX) $(CFLAGS) $(FFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	$(QUIET) $(ECHO) "  [RM] $(OBJS) $(BINARYNAME) $(CLEANUP)"
	$(QUIET) rm -f $(OBJS) $(BINARYNAME) $(CLEANUP)

install:
	install -c -D -m 744 $(BINARYNAME) $(BINARY_DIR)

