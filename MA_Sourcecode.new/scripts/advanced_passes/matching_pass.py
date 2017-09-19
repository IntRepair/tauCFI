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

		for entry in clang_index:
			if not entry in padyn_index:
				print index_name + ":", "In clang but not in padyn", entry
			else:
				both_index += [entry]

		for entry in padyn_index:
			if not entry in clang_index:
				print index_name + ":", "In padyn but not in clang", entry

		print index_name + ":", "fns in both:", len(both_index)
		return both_index

	def _run_on_target(self, target, padyn, clang):
		print "clang", clang.keys()
		print "padyn", padyn.keys()
		self._get_both_fns(padyn, clang, "at_index")
		self._get_both_fns(padyn, clang, "ct_index")
