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

llvm_source_dir = os.path.join(project_root, config["type_shield.llvm.source_dir"])
print "Using llvm sources from", llvm_source_dir

llvm_build_dir = os.path.join(project_root, config["type_shield.llvm.build_dir"])
print "Using llvm build dir", llvm_build_dir

if os.path.exists(llvm_build_dir):
	if arguments["force"]:
		print "Removing build directory", llvm_build_dir
		command_string= "rm -r " + llvm_build_dir
		print command_string
		os.system(command_string)
	else:
		print "Build directory", llvm_build_dir, "already exists for llvm, aborting..."
		exit(-1)

command_string = "mkdir -p " + llvm_build_dir + " ; "
command_string += "cd " + llvm_build_dir + " ; "

cmake_flags = []
cmake_flags += ["-DCMAKE_BUILD_TYPE=Release"]
cmake_flags += ["-DCMAKE_CXX_COMPILER=" + config["type_shield.cxxcompiler"] ]
cmake_flags += ["-DCMAKE_C_COMPILER=" + config["type_shield.ccompiler"] ]

command_string += build_cmake_command(target=llvm_source_dir, pre=cmake_flags) + "; "
command_string += build_cmake_command(target=".", pre=["--build"], post=["-- -j4"]) + "; "
command_string += "cd " + os.path.join(llvm_build_dir, "bin") + "; "
command_string += "ln lld ld"

print command_string
os.system(command_string)

