#
# Makefile for specinvoke on Windows with MinGW GCC
# Copyright 2009 Standard Performance Evaluation Corporation
#
# $Id: Makefile 6361 2011-03-03 22:29:36Z cloyce $

all: specinvoke.exe specinvoke_pm.exe

CC=gcc
CFLAGS=-O3 -Wall -Wextra -I. -I..

# If anyone ever writes a performance monitor module for Windows, put the
# name of its directory here:
PM_DIR=../pm_stub

SOURCES = ../specinvoke.c ../win32.c ../getopt.c
OBJS = $(SOURCES:.c=.o)
PM_OBJS = $(SOURCES:.c=.pm.o) $(PM_DIR)/pmfuncs.o

specinvoke.exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

specinvoke_pm.exe: $(PM_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PM_OBJS)

%.pm.o: %.c config.h ../specinvoke.h $(PM_DIR)/specinvoke_pm.h
	$(CC) $(CFLAGS) -c -o $@ -I$(PM_DIR) -DPERFMON $<

$(PM_DIR)/pmfuncs.o: $(PM_DIR)/specinvoke_pm.h
	$(MAKE) -C $(PM_DIR) -f Makefile.MinGW

cleanobjs:
	del /s /q /f $(subst /,\\,$(OBJS)) $(subst /,\\,$(PM_OBJS)) ..\*.pdb ..\*.ilk ..\*.obj

clean: cleanobjs
	del specinvoke.exe specinvoke_pm.exe

distclean: clean
