PROJECT_ROOT=/home/matthias/typeshield
SCRIPT_DIR=$(PROJECT_ROOT)/scripts
BINARY_DIR=$(PROJECT_ROOT)/bin
PROJECT_SOURCES=$(PROJECT_ROOT)/src
DYNINST_ROOT=$(PROJECT_ROOT)/Dyninst
DYNAMORIO_ROOT=$(PROJECT_ROOT)/DynamoRIO
CC=gcc
CXX=g++

all:
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/libTypeShield
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/analyzer

install: all
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/libTypeShield install
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/analyzer install

clean:
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/libTypeShield clean
	+$(MAKE) -C $(PROJECT_SOURCES)/cxx/analyzer clean
