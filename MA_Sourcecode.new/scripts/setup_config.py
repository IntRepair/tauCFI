import os
import sys

from parse_config import parse_config
from parse_arguments import parse_arguments

arguments = parse_arguments(sys.argv[1:], flag_options=["force"], value_options=["config", "base-config", "project-root"])
base_config_path=arguments["base-config"]
config_path=arguments["config"]

project_root=arguments["project-root"]
print "Using project root:", project_root

base_config_path = os.path.join(project_root, base_config_path)
config_path = os.path.join(project_root, config_path)


if not os.path.exists(base_config_path):
	print "Base Config (" + base_config_path + ") does not exist, aborting..."

if not arguments["force"] and os.path.exists(config_path):
	print "Config (" + config_path + ") already exists, aborting..."
	exit(-1)

command_string = "cp " + base_config_path + " " + config_path

print command_string
os.system(command_string)
