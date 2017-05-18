#!/bin/bash

LLVMDIR=$HOME/typearmor/build_llvm
BINUTILS=$HOME/binutils_build
SOURCEDIR=$HOME/typearmor/src_tests/
TESTDIR=$HOME/typearmor/tests
ANALYSIS=$HOME/typearmor/run-app.sh

mkdir $TESTDIR -p

python src/py/test_verify_matching.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
python src/py/test_verify_classification.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
python src/py/test_verify_classification_type.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
python src/py/test_verify_classification_type_count.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
#python src/py/test_generate_policy_compare.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
python src/py/test_generate_policy_compare_at.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR
#python src/py/test_generate_baseline_svg.py --analysis-script=$ANALYSIS --binutils-dir=$BINUTILS --llvm-build-dir=$LLVMDIR --source-dir=$SOURCEDIR --test-dir=$TESTDIR

