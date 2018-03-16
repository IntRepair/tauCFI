import sys
import os

from parse_config import parse_config
from parse_arguments import parse_arguments

from base_functionality.test_target import get_targets

# these are "basic" passes as they only need the basic system
from base_passes.compile_pass import CompilePass
from base_passes.analysis_pass import AnalysisPass
# these are "advanced" passes as they need access to more advanced systems
from advanced_passes.matching_pass import MatchingPass
#from advanced_passes.precision_count_pass import PrecisionCountPass
#from advanced_passes.precision_type_pass import PrecisionTypePass
#from advanced_passes.pairing_count_pass import PairingCountPass
#from advanced_passes.pairing_type_pass import PairingTypePass

_available_passes = {}
_available_passes["compile"] = CompilePass
_available_passes["analysis"] = AnalysisPass
_available_passes["matching"] = MatchingPass
#_available_passes["precision_count"] = PrecisionCountPass
#_available_passes["precision_type"] = PrecisionTypePass
#_available_passes["pairing_count"] = PairingCountPass
#_available_passes["pairing_type"] = PairingTypePass

def main(argv):
	arguments = parse_arguments(sys.argv[1:], value_options=["config"], flag_options=["force"])
	config = parse_config("type_shield", arguments["config"])

	project_root = config["type_shield.dir"]
	print "Using project root:", project_root

	source_dir = os.path.join(project_root, config["type_shield.sources.dir"])

	test_sources_dir = os.path.join(source_dir, config["type_shield.sources.tests.dir"])
	print "Using test program sources from", test_sources_dir

	test_sources_config_path = os.path.join(test_sources_dir, config["type_shield.sources.tests.config"])
	print "Using test program config", test_sources_config_path

	test_sources_config = parse_config("test_sources", test_sources_config_path)
	test_targets = get_targets(config, test_sources_config)
	if test_targets == []:
		print "[Error - Runtime] no test targets configured"
		print "[Fatal Error] invalid configuration terminating application!"
		return

	pass_order = config["type_shield.pass.order"].split()

	invalid_passes = [x for x in pass_order if x not in _available_passes.keys()]
	if invalid_passes != []:
		print "[Error - Runtime] invalid passes: ", str(invalid_passes)
		print "[Fatal Error] invalid configuration terminating application!"
		return

	print "Running passes:"
	for pass_name in pass_order:
		print "\t" + pass_name
	
	for pass_name in pass_order:
		print "Running pass", pass_name, "..."
		runtime_pass = (_available_passes[pass_name])(config, arguments)
		runtime_pass.run_on_targets(test_targets)

main(sys.argv[1:])
