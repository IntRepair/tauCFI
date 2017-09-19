import os
import sys

from parse_config import parse_config
from parse_arguments import parse_arguments

def build_makefile_header(config):
	header  = "PROJECT_ROOT=" + config["type_shield.dir"] + "\n"
	header += "SCRIPT_DIR=$(PROJECT_ROOT)/" + config["type_shield.script.dir"] + "\n"
	header += "BINARY_DIR=$(PROJECT_ROOT)/" + config["type_shield.binary.dir"] + "\n"
	header += "PROJECT_SOURCES=$(PROJECT_ROOT)/" + config["type_shield.sources.dir"] + "\n"
	header += "DYNINST_ROOT=$(PROJECT_ROOT)/" + config["type_shield.dyninst.dir"] + "\n"
	header += "DYNAMORIO_ROOT=$(PROJECT_ROOT)/" + config["type_shield.dynamorio.dir"] + "\n"
	header += "CC=" + config["type_shield.ccompiler"] + "\n"
	header += "CXX=" + config["type_shield.cxxcompiler"] + "\n"
	return header

def build_main_makefile(config, build_targets):
	makefile = build_makefile_header(config) + "\n"
	makefile += "all:\n"
	for build_target in build_targets:
		makefile += "\t+$(MAKE) -C $(PROJECT_SOURCES)/" + build_target + "\n"
	makefile += "\n"
	makefile += "install: all\n"
	for build_target in build_targets:
		makefile += "\t+$(MAKE) -C $(PROJECT_SOURCES)/" + build_target + " install\n"
	makefile += "\n"
	makefile += "clean:\n"
	for build_target in build_targets:
		makefile += "\t+$(MAKE) -C $(PROJECT_SOURCES)/" + build_target + " clean\n"
	return makefile

arguments = parse_arguments(sys.argv[1:], flag_options=["force"], value_options=["config"])
config = parse_config("type_shield", arguments["config"])

#TODO: Make this configurable
build_targets = ["cxx/libTypeShield", "cxx/analyzer"]

project_root = config["type_shield.dir"]
print "Using project root:", project_root

source_dir = os.path.join(project_root, config["type_shield.sources.dir"])
print "Using project sources:", source_dir

main_makefile_path="Makefile"
main_makefile_path=os.path.join(project_root, main_makefile_path)

build_target_paths = [os.path.join(source_dir,  x) for x in build_targets]

if not arguments["force"] and os.path.exists(main_makefile_path):
	print "Makefile", main_makefile_path, "already exists, aborting..."
	exit(-1)

with open(main_makefile_path, "w") as main_makefile:
	main_makefile.write(build_main_makefile(config, build_targets))

for build_target_path in build_target_paths:
	makefile_include_path=os.path.join(build_target_path, "Makefile.inc")
	with open(makefile_include_path, "w") as makefile_include:
		makefile_include.write(build_makefile_header(config))

