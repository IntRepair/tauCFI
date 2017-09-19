#!/bin/bash

BASE_CONFIG=config/base_config.xml
CONFIG=config/config.xml
SCRIPTS=scripts

python $SCRIPTS/setup_config.py --project-root=`pwd` --base-config=$BASE_CONFIG --config=$CONFIG $*
python $SCRIPTS/setup_dyninst.py --config=$CONFIG $*
python $SCRIPTS/setup_llvm.py --config=$CONFIG $*
python $SCRIPTS/setup_test_programs.py --config=$CONFIG $*
python $SCRIPTS/setup_typeshield.py --config=$CONFIG $*

