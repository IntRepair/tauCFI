import os

def get_string(config, key):
	result = ""
	if key in config.keys():
		result = config[key]
	return result

class Target:
    def __init__(self, name, method):
        self.name = name
        self.method = method

    def set_sourcedir(self, sourcedir):
        self.sourcedir = sourcedir

    def set_binary_path(self, binary_path):
        self.binary_path = binary_path
        self.binary_name = os.path.basename(binary_path)

    def set_sourcedir_binary(self, sourcedir_binary):
        self.sourcedir_binary = sourcedir_binary

    def set_prefix(self, prefix):
        self.prefix = prefix

    def set_cc_options(self, cc_options):
        self.cc_options = cc_options

    def set_cxx_options(self, cxx_options):
	self.cxx_options = cxx_options

    def set_ld_options(self, ld_options):
        self.ld_options = ld_options

def get_targets(config, test_source_config):
	project_root = config["type_shield.dir"]	
	source_dir = os.path.join(project_root, config["type_shield.sources.dir"])
	test_sources_dir = os.path.join(source_dir, config["type_shield.sources.tests.dir"])
	test_dir = os.path.join(project_root, config["type_shield.test.dir"])

	test_targets = []
	for program in test_source_config["programs"]:
		target = Target(program["name"], program["method"])
		target.set_sourcedir(os.path.join(test_sources_dir, program["src"]))
		target.set_binary_path(os.path.join(test_sources_dir, program["binary_path"]))
		target.set_sourcedir_binary(program["sourcedir_binary"])
		target.set_prefix(os.path.join(test_dir, program["name"]))
		target.set_cc_options(get_string(config, "type_shield.sources.tests.cc_options") + get_string(program, "cc_options"))
		target.set_cxx_options(get_string(config, "type_shield.sources.tests.cxx_options") + get_string(program, "cxx_options"))
		target.set_ld_options(get_string(config, "type_shield.sources.tests.ld_options") + get_string(program, "ld_options"))

		test_targets += [target]

	return test_targets
