
from advanced_pass import AdvancedPass as AdvancedPass
from parsing.parse_verify import parse_verify as parse_verify_tag

import advanced_functionality.matching as matching
import base_functionality.utilities as utils

def entry_identity(entry):
    return (entry["demang_name"], entry["mang_name"])

def build_lookup_padyn(source_key, padyn, cfn_acc_padyn_clang):
	lookup_padyn = dict([(entry_identity(entry), entry) for entry in padyn[source_key]])

	for delete_key in (set(lookup_padyn.keys()) - set([key_pair[0] for key_pair in cfn_acc_padyn_clang])):
		del lookup_padyn[delete_key]

	return lookup_padyn

def build_lookup_clang(source_key, clang, cfn_acc_padyn_clang):
	lookup_clang = dict([(entry_identity(entry), entry) for entry in clang[source_key]])

	for delete_key in (set(lookup_clang.keys()) - set([key_pair[1] for key_pair in cfn_acc_padyn_clang])):
		del lookup_clang[delete_key]

	return lookup_clang

def arg_count(entry):
	return min(entry["param_count"], 6)

def ret_type_void(entry):
	return entry["ret_type"][1]["is_void"]

def compiled_fn_precision(padyn, clang, cfn_acc_padyn_clang):
	padyn_lookup = build_lookup_padyn("compiled_function", padyn, cfn_acc_padyn_clang)
	clang_lookup = build_lookup_clang("compiled_function", clang, cfn_acc_padyn_clang)

	cfn_perfect_args = []
	cfn_problem_args = []
	cfn_perfect_ret = []
	cfn_problem_ret = []

	for key_pair in cfn_acc_padyn_clang:
		arg_count_padyn = arg_count(padyn_lookup[key_pair[0]])
		arg_count_clang = arg_count(clang_lookup[key_pair[1]])

		if arg_count_padyn == arg_count_clang:
			cfn_perfect_args += [key_pair]
		elif arg_count_padyn > arg_count_clang:
			cfn_problem_args += [(key_pair, padyn_lookup[key_pair[0]], clang_lookup[key_pair[1]])]

		ret_padyn = ret_type_void(padyn_lookup[key_pair[0]])
		ret_clang = ret_type_void(clang_lookup[key_pair[1]])

		if ret_padyn == ret_clang:
			cfn_perfect_ret += [key_pair]
		elif ret_padyn and not ret_clang:
			cfn_problem_ret += [(key_pair, padyn_lookup[key_pair[0]], clang_lookup[key_pair[1]])]

	return (cfn_perfect_args, cfn_problem_args, cfn_perfect_ret, cfn_problem_ret) 

def call_site_precision(padyn, clang, cs_acc_padyn_clang):
	cs_perfect_args = []
	cs_problem_args = []
	cs_perfect_ret = []
	cs_problem_ret = []

	count = 0

	for (key_pair, css_padyn, css_clang) in cs_acc_padyn_clang:
		for index, (cs_padyn, cs_clang) in enumerate(zip(css_padyn, css_clang)):
			count += 1
			arg_count_padyn = arg_count(cs_padyn)
			arg_count_clang = arg_count(cs_clang)

			if arg_count_padyn == arg_count_clang:
				cs_perfect_args += [(key_pair, index)]
			elif arg_count_padyn < arg_count_clang:
				cs_problem_args += [(key_pair, index, cs_padyn, cs_clang)]

			ret_padyn = ret_type_void(cs_padyn)
			ret_clang = ret_type_void(cs_clang)

			if ret_padyn == ret_clang:
				cs_perfect_ret += [(key_pair, index)]
			elif not ret_padyn and ret_clang:
				cs_problem_ret += [(key_pair, index, cs_padyn, cs_clang)]

	return (cs_perfect_args, cs_problem_args, cs_perfect_ret, cs_problem_ret)

class PrecisionCountPass(AdvancedPass):
	def __init__(self, config, args):
		AdvancedPass.__init__(self, "precision_count", config, args)
		fieldnames = ["opt"]
		fieldnames += ["target"]

		subfieldnames = ["args perfect", "args perfect pct", "args problem", "args problem pct"]
		subfieldnames2 = ["void correct", "void correct pct", "void problem", "void problem pct"]

		fieldnames += ["cs"] + ["cs " + s for s in subfieldnames]
		fieldnames += ["cs non-" + s for s in subfieldnames2]
		fieldnames += ["ct"] + ["ct " + s for s in subfieldnames]
		fieldnames += ["ct " + s for s in subfieldnames2]

		config_prefixes = config["type_shield.pass.precision_count.prefixes"].split(" ")
		assert(len(config_prefixes) > 0)

		self.prefixes = [prefix + "count" for prefix in config_prefixes]

		for prefix in self.prefixes:
			self.csv_set_define("precision." + prefix, fieldnames)

	def _run_on_target(self, target, padyn_base, clang):
		prg_name = target.name

		for prefix in self.prefixes:
			padyn = parse_verify_tag(target.prefix, target.binary_name, prefix, prefix)
			csv_data = {}
			csv_data["opt"] = "O2"
			csv_data["target"] = prg_name

			(cfn_acc_padyn_clang, _, _) = matching.compiled_function_matching(padyn, clang)
			(cs_acc_padyn_clang, _, _) = matching.call_site_matching(padyn, clang, cfn_acc_padyn_clang)

			cs_acc_count = sum([ len(css_padyn) for (cfn, css_padyn, css_clang) in cs_acc_padyn_clang ])

			csv_data["ct"] = len(cfn_acc_padyn_clang)
			csv_data["cs"] = cs_acc_count

			(cfn_perfect_args, cfn_problem_args, cfn_perfect_ret, cfn_problem_ret) = compiled_fn_precision(padyn, clang, cfn_acc_padyn_clang)

			self.log_errors_target(prg_name, [prefix + "CFN arg problematic CLANG[" + str(arg_count(clang_cfn)) + "] PADYN[" + str(arg_count(padyn_cfn)) + "] for [" + str(key_pair) + "]:\n\t" + str(padyn_cfn) + "\n\t" + str(clang_cfn) for (key_pair, padyn_cfn, clang_cfn) in cfn_problem_args])

			self.log_errors_target(prg_name, [prefix + "CFN ret problematic CLANG[" + str(ret_type_void(clang_cfn)) + "] PADYN[" + str(ret_type_void(padyn_cfn)) + "] for [" + str(key_pair) + "]:\n\t" + str(padyn_cfn) + "\n\t" + str(clang_cfn) for (key_pair, padyn_cfn, clang_cfn) in cfn_problem_ret])

			csv_data["ct args perfect"] = len(cfn_perfect_args)
			csv_data["ct args perfect pct"] = utils.gen_pct_string(float(len(cfn_perfect_args)) / float(len(cfn_acc_padyn_clang)))
			csv_data["ct args problem"] = len(cfn_problem_args)
			csv_data["ct args problem pct"] = utils.gen_pct_string(float(len(cfn_problem_args)) / float(len(cfn_acc_padyn_clang)))

			csv_data["ct void correct"] = len(cfn_perfect_ret)
			csv_data["ct void correct pct"] = utils.gen_pct_string(float(len(cfn_perfect_ret)) / float(len(cfn_acc_padyn_clang)))
			csv_data["ct void problem"] = len(cfn_problem_ret)
			csv_data["ct void problem pct"] = utils.gen_pct_string(float(len(cfn_problem_ret)) / float(len(cfn_acc_padyn_clang)))

			(cs_perfect_args, cs_problem_args, cs_perfect_ret, cs_problem_ret) = call_site_precision(padyn, clang, cs_acc_padyn_clang)

			self.log_errors_target(prg_name, [prefix + "CS arg problematic CLANG[" + str(arg_count(cs_clang)) + "] PADYN[" + str(arg_count(cs_padyn)) + "] for [" + str(key_pair) + "]:\n\t" + str(cs_padyn) + "\n\t" + str(cs_clang) for (key_pair, index, cs_padyn, cs_clang) in cs_problem_args])

			self.log_errors_target(prg_name, [prefix + "CS ret problematic CLANG[" + str(ret_type_void(cs_clang)) + "] PADYN[" + str(ret_type_void(cs_padyn)) + "] for [" + str(key_pair) + "]:\n\t" + str(cs_padyn) + "\n\t" + str(cs_clang) for (key_pair, index, cs_padyn, cs_clang) in cs_problem_ret])

			csv_data["cs args perfect"] = len(cs_perfect_args)
			csv_data["cs args perfect pct"] = utils.gen_pct_string(float(len(cs_perfect_args)) / float(cs_acc_count))
			csv_data["cs args problem"] = len(cs_problem_args)
			csv_data["cs args problem pct"] = utils.gen_pct_string(float(len(cs_problem_args)) / float(cs_acc_count))

			csv_data["cs non-void correct"] = len(cs_perfect_ret)
			csv_data["cs non-void correct pct"] = utils.gen_pct_string(float(len(cs_perfect_ret)) / float(cs_acc_count))
			csv_data["cs non-void problem"] = len(cs_problem_ret)
			csv_data["cs non-void problem pct"] = utils.gen_pct_string(float(len(cs_problem_ret)) / float(cs_acc_count))

			self.csv_output_row("precision." + prefix, csv_data)
