import os

from test_pass import Pass

class AnalysisPass(Pass):
    def __init__(self, config, args):
        Pass.__init__(self, "analyze", config, args)
        self._options = ["--calltargets", "--callsites"]

    def _run_on_target(self, target):
        if not os.path.exists(target.prefix):
            os.makedirs(target.prefix)

        #if not os.path.exists(os.path.join(target.prefix, analysis.info))
        #    test_config.get_analyzer()

	project_dir = self.config["type_shield.dir"]

	source_dir = os.path.join(project_dir, self.config["type_shield.sources.dir"])
	binary_dir = os.path.join(project_dir, self.config["type_shield.binary.dir"])
	dyninst_dir = os.path.join(project_dir, self.config["type_shield.dyninst.dir"])
	dyninst_rt_path = os.path.join(dyninst_dir, self.config["type_shield.dyninst.rt_library"])
	dynamorio_dir = os.path.join(project_dir, self.config["type_shield.dynamorio.dir"])
	dynamorio_libs = os.path.join(dynamorio_dir, self.config["type_shield.dynamorio.lib"])

        command_string = "cd " + target.prefix + "; "
        command_string += "sudo "
        command_string += "DYNINSTAPI_RT_LIB=" + dyninst_rt_path + " "
        command_string += "LD_LIBRARY_PATH=" + binary_dir + ":" + dynamorio_libs + ":" + "$LD_LIBRARY_PATH" + " "
	command_string += os.path.join(binary_dir, "analyzer") + " "
        command_string += os.path.join(project_dir, target.prefix, target.binary_name)
        for option in self._options:
            command_string += " " + option

	command_string += "; " + command_string + " --types"

	print command_string
        os.system(command_string)

    def _run_on_targets(self, targets):
        for target in targets:
            self._run_on_target(target)
