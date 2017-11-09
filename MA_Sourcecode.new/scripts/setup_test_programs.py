import os
import sys

from parse_config import parse_config
from parse_arguments import parse_arguments

def sanitize_mysql_pp(file_path):
	with  open(file_path, "r") as f_in:
		with open(file_path + ".cpy", "w") as f_out:
			for line in f_in:
				if not line.startswith("#include"):
					f_out.write(line)

	os.rename(file_path + ".cpy", file_path)

arguments = parse_arguments(sys.argv[1:], flag_options=["force"], value_options=["config"])
config = parse_config("type_shield", arguments["config"])

project_root = config["type_shield.dir"]
print "Using project root:", project_root

source_dir = config["type_shield.sources.dir"]

test_sources_dir = os.path.join(source_dir, config["type_shield.sources.tests.dir"])
print "Using test program sources from", test_sources_dir

test_sources_config_path = os.path.join(test_sources_dir, config["type_shield.sources.tests.config"])
print "Using test program config", test_sources_config_path

test_sources_config = parse_config("test_sources", test_sources_config_path)

command_string = "cd " + str(test_sources_dir) + "; "

for program_info in test_sources_config["programs"]:
	command_string += "("
	command_string += "tar -xzvf " + str(program_info["archive"]) + " >> /dev/null;"
	command_string += " echo \"Extracted " + program_info["src"] + "\""
	command_string += ") ;"	

os.system(command_string)

for program_info in test_sources_config["programs"]:
	if program_info["name"] == "mysql":
		#sanitize ./include/mysql.h.pp
		sanitize_mysql_pp(os.path.join(project_root, test_sources_dir, program_info["src"], "include", "mysql.h.pp"))

