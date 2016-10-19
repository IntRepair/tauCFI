#!/bin/bash

LLVMDIR=$HOME/patharmor/build_llvm
BINUTILS=$HOME/binutils_build
SOURCEDIR=$HOME/patharmor/src_tests/
TESTDIR=$HOME/patharmor/tests
ANALYSIS=$HOME/patharmor/run-app.sh

mkdir $TESTDIR -p

python src/py/test_verify_matching.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR

