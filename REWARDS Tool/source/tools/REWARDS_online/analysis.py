#!/usr/bin/env python3

import sys
import os
import subprocess

if len(sys.argv) < 2:
    print ("Usage: %s [directory containing trace] .." % os.path.basename(sys.argv[0]))
    sys.exit(1)

path = os.path.dirname(os.path.abspath(sys.argv[0]))

subprocess.call([path + "/analysis_i.py"] + sys.argv[1:])
subprocess.call([path + "/analysis_i.py"])
os.remove("int.out")
