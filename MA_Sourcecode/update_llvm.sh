#!/bin/bash

llvm="`pwd`/llvm"

cd $llvm
svn update

cd $llvm/tools/lld
svn update

cd $llvm/tools/clang
svn update

cd $llvm/tools/clang/extra
svn update

cd $llvm/projects/libcxx
svn update

cd $llvm/projects/compiler-rt
svn update

cd $llvm/projects/libcxxabi
svn update

