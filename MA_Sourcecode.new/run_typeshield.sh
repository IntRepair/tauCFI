#!/bin/bash

CONFIG=config/config.xml
SCRIPTS=scripts

python $SCRIPTS/run_typeshield.py --config=$CONFIG $*

