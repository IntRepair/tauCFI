import os
import sys

from parse_config import parse_config
from parse_arguments import parse_arguments

def build_cmake_command(pre, target, post=[]):
        command_string = "cmake "

        for flag in pre:
                command_string += flag
                command_string += " "
        command_string += target
	for flag in post:
		command_string += " "
		command_string += flag
        return command_string

arguments = parse_arguments(sys.argv[1:], flag_options=["force"], value_options=["config"])
config = parse_config("type_shield", arguments["config"])

project_root = config["type_shield.dir"]
print "Using project root:", project_root

dyninst_sources = config["type_shield.dyninst.dir"]
dyninst_sources_path = os.path.join(project_root, dyninst_sources)
print "Using dyninst root at", dyninst_sources_path

command_string = "cd " + dyninst_sources_path + "; "

cmake_flags = []
cmake_flags += ["-DCMAKE_BUILD_TYPE=Release"]
cmake_flags += ["-DCMAKE_CXX_COMPILER=clang++"]
cmake_flags += ["-DCMAKE_C_COMPILER=clang"]

command_string += build_cmake_command(target=".", pre=cmake_flags) + "; "
command_string += build_cmake_command(target=".", pre=["--build"], post=["-- -j4"])

print command_string
os.system(command_string)
