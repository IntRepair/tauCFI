from advanced_pass import AdvancedPass as AdvancedPass
from advanced_functionality.verify_matching import verify as verify_matching
import base_functionality.utilities as utils

class MatchingPass(AdvancedPass):
	def __init__(self, config, args):
		AdvancedPass.__init__(self, "matching", config, args)

	def _get_both_fns(self, padyn, clang, index_name):
		both_index = []
		clang_index = clang[index_name]
		padyn_index = padyn[index_name]

		clang_index_demang = {}
		padyn_index_demang = {}

		clang_missing = {}
		padyn_missing = {}

		for entry in clang_index:
			utils.add_values_to_key(clang_index_demang, entry[0], entry)
			if not entry in padyn_index:
				# DEBUG
				#print index_name + ":", "In clang but not in padyn", entry
				utils.add_values_to_key(padyn_missing, entry[0], entry)
			else:
				both_index += [entry]

		for entry in padyn_index:
			utils.add_values_to_key(padyn_index_demang, entry[0], entry)
			if not entry in clang_index:
				# DEBUG
				#print index_name + ":", "In padyn but not in clang", entry
				utils.add_values_to_key(clang_missing, entry[0], entry)

		for key in clang_missing.keys():
			if key in padyn_missing.keys():
				padyn_indices_key = padyn_missing[key]
				clang_indices_key = clang_missing[key]

				if len(padyn_indices_key) == 1 and len(clang_indices_key) == 1:
					both_index += [(key, [clang_indices_key[0][1], padyn_indices_key[0][1]])]
				else:
					print key, "missing in both with problems"
					print "\t", padyn_indices_key
					print "\t", clang_indices_key
			else:
				print key, "truly missing in clang"

		for key in padyn_missing.keys():
			if key not in clang_missing.keys():
				print key, "truly missing in padyn"

		print index_name + ":", "fns in both:", len(both_index)
		return both_index

	def _run_on_target(self, target, padyn, clang):
		problem_string = ""

		for key in ["at_index", "cfn_index"]

		for cfn_index in clang["double_vals"]["cfn_index"]:
			print "cfn_index", cfn_index

		for cfn in clang["double_vals"]["compiled_function"]:
			print "cfn", cfn

		both_fns = self._get_both_fns(padyn, clang, "cfn_index")

