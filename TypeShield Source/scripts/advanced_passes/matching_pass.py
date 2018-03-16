from advanced_pass import AdvancedPass as AdvancedPass
#from advanced_functionality.verify_matching import verify as verify_matching
import base_functionality.utilities as utils

def group_identities_key0(identities):
	identities_map = {}

	for identity in list(identities):
		utils.add_values_to_key(identities_map, identity[0], identity)

	return identities_map

def compiled_function_matching(padyn, clang):
	padyn_cfns = set(padyn["cfn_index"])
	clang_cfns = set(clang["cfn_index"])

	padyn_not_in_clang = padyn_cfns - clang_cfns
	clang_not_in_padyn = clang_cfns - padyn_cfns

	acceptable = padyn_cfns.intersection(clang_cfns)

	padyn_not_in_clang_map = group_identities_key0(padyn_not_in_clang)
	clang_not_in_padyn_map = group_identities_key0(clang_not_in_padyn)

	acceptable_padyn = set(list(acceptable))
	acceptable_clang = set(list(acceptable))

	for key in list(set(clang_not_in_padyn_map.keys()).intersection(set(padyn_not_in_clang_map.keys()))):
		padyn_identities = padyn_not_in_clang_map[key]
		clang_identities = clang_not_in_padyn_map[key]

		if len(clang_identities) == 1 and len(padyn_identities) == 1:
			acceptable_padyn.union(set(padyn_identities))
			padyn_not_in_clang -= set(padyn_identities)

			acceptable_clang.union(set(clang_identities))
			clang_not_in_padyn -= set(clang_identities)

	return (acceptable_padyn, acceptable_clang, padyn_not_in_clang, clang_not_in_padyn)

def address_taken_matching(padyn, clang, cfn_acc_padyn, cfn_acc_clang):
	padyn_ats = set(padyn["at_index"]).intersection(cfn_acc_padyn)
	clang_ats = set(clang["at_index"]).intersection(cfn_acc_clang)

	padyn_not_in_clang = padyn_ats - clang_ats
	clang_not_in_padyn = clang_ats - padyn_ats

	acceptable = padyn_ats.intersection(clang_ats)

	padyn_not_in_clang_map = group_identities_key0(padyn_not_in_clang)
	clang_not_in_padyn_map = group_identities_key0(clang_not_in_padyn)

	acceptable_padyn = set(list(acceptable))
	acceptable_clang = set(list(acceptable))

	for key in list(set(clang_not_in_padyn_map.keys()).intersection(set(padyn_not_in_clang_map.keys()))):
		padyn_identities = padyn_not_in_clang_map[key]
		clang_identities = clang_not_in_padyn_map[key]

		if len(clang_identities) == 1 and len(padyn_identities) == 1:
			acceptable_padyn.union(set(padyn_identities))
			padyn_not_in_clang -= set(padyn_identities)

			acceptable_clang.union(set(clang_identities))
			clang_not_in_padyn -= set(clang_identities)

	return (acceptable_padyn, acceptable_clang, padyn_not_in_clang, clang_not_in_padyn)

def call_site_matching(padyn, clang, cfn_acc_padyn, cfn_acc_clang):
	padyn_css = padyn.get("cs_index", {})
	clang_css = clang.get("cs_index", {})

	cfn_cs_padyn_keys = set(padyn_css.keys()).intersection(cfn_acc_padyn)
	cfn_cs_clang_keys = set(clang_css.keys()).intersection(cfn_acc_clang)

	cfn_cs_both_keys = cfn_cs_padyn_keys.intersection(cfn_cs_clang_keys)

	cs_acc_padyn = []
	cs_acc_clang = []

	cs_disc_padyn = []
	cs_disc_clang = []

	for cfn_cs_key in cfn_cs_both_keys:
		if len(padyn_css[cfn_cs_key]) == len(clang_css[cfn_cs_key]):
			cs_acc_padyn += [(cfn_cs_key, padyn_css[cfn_cs_key])]
			cs_acc_clang += [(cfn_cs_key, clang_css[cfn_cs_key])]
		else:
			cs_disc_padyn += [(cfn_cs_key, padyn_css[cfn_cs_key])]
			cs_disc_clang += [(cfn_cs_key, clang_css[cfn_cs_key])]

	for cfn_cs_key in cfn_cs_padyn_keys - cfn_cs_both_keys:
		cs_disc_padyn += [(cfn_cs_key, padyn_css[cfn_cs_key])]

	for cfn_cs_key in cfn_cs_clang_keys - cfn_cs_both_keys:
		cs_disc_clang += [(cfn_cs_key, clang_css[cfn_cs_key])]

	return (cs_acc_padyn, cs_acc_clang, cs_disc_padyn, cs_disc_clang)

class MatchingPass(AdvancedPass):
	def __init__(self, config, args):
		AdvancedPass.__init__(self, "matching", config, args)

		fieldnames = ["target"]
		fieldnames += ["opt"]

		subfieldnames = ["clang", "clang pct", "padyn", "padyn pct"]

		fieldnames += ["fn"] + ["fn not in " + s for s in subfieldnames]
		fieldnames += ["at"] + ["at not in " + s for s in subfieldnames]
		fieldnames += ["cs"] + ["cs discarded " + s for s in subfieldnames]

		self.csv_set_define("matching", fieldnames)

	def _run_on_target(self, target, padyn, clang):
		csv_data = {}

		prg_name = target.name
		double_vals = clang["double_vals"]

		for key in ["address_taken", "compiled_function", "call_site"]:
			if key in double_vals.keys():
                                self.log_errors_target(prg_name, ["double_vals[" + key + "]: " + str(entry) for entry in double_vals[key]])

		csv_data["target"] = prg_name
		csv_data["opt"] = "O2"

		(cfn_acc_padyn, cfn_acc_clang, cfn_not_in_clang, cfn_not_in_padyn) = compiled_function_matching(padyn, clang)
		cfn_count = len(cfn_acc_padyn)
		assert(len(cfn_acc_padyn) == len(cfn_acc_clang))
		csv_data["fn"] = cfn_count
		cfn_not_clang = float(len(cfn_not_in_clang))
		csv_data["fn not in clang"] = str(int(cfn_not_clang))
		csv_data["fn not in clang pct"] = utils.gen_pct_string(cfn_not_clang / (cfn_not_clang + cfn_count))
		cfn_not_padyn = float(len(cfn_not_in_padyn))
		csv_data["fn not in padyn"] = str(int(cfn_not_padyn))
		csv_data["fn not in padyn pct"] = utils.gen_pct_string(cfn_not_padyn / (cfn_not_padyn + cfn_count))

                self.log_errors_target(prg_name, ["padyn cFN not in clang: " + fun[0] + " " + fun[1] for fun in cfn_not_in_clang])
		self.log_errors_target(prg_name, ["clang cFN not in padyn: " + fun[0] + " " + fun[1] for fun in cfn_not_in_padyn])

		# We have a problem when an AT is in clang but not in padyn (after we reduced to fn_acceptable)
		(at_acc_padyn, at_acc_clang, at_not_in_clang, at_not_in_padyn) = address_taken_matching(padyn, clang, cfn_acc_padyn, cfn_acc_clang)
		at_count = len(at_acc_padyn)
		assert(len(at_acc_padyn) == len(at_acc_clang))
		csv_data["at"] = at_count
		at_not_clang = float(len(at_not_in_clang))
		csv_data["at not in clang"] = str(int(at_not_clang))
		csv_data["at not in clang pct"] = utils.gen_pct_string(at_not_clang / (at_not_clang + at_count))
		at_not_padyn = float(len(at_not_in_padyn))
		csv_data["at not in padyn"] = str(int(at_not_padyn))
		csv_data["at not in padyn pct"] = utils.gen_pct_string(at_not_padyn / (at_not_padyn + at_count))

		self.log_errors_target(prg_name, ["AT not in clang: " + fun[0] + " " + fun[1] for fun in at_not_in_clang])
		self.log_errors_target(prg_name, ["AT not in padyn: " + fun[0] + " " + fun[1] for fun in at_not_in_padyn])

		(cs_acc_padyn, cs_acc_clang, cs_disc_padyn, cs_disc_clang) = call_site_matching(padyn, clang, cfn_acc_padyn, cfn_acc_clang)

		cs_acc_count = sum([ len(css) for (cfn, css) in cs_acc_padyn ])
		csv_data["cs"] = str(cs_acc_count)

		cs_disc_clang_count = sum([ len(css) for (cfn, css) in cs_disc_clang ])
		cs_disc_padyn_count = sum([ len(css) for (cfn, css) in cs_disc_padyn ])

		csv_data["cs discarded clang"] = str(cs_disc_clang_count)
		csv_data["cs discarded clang pct"] = utils.gen_pct_string(float(cs_disc_clang_count) / float(cs_disc_clang_count + cs_acc_count))
		csv_data["cs discarded padyn"] = str(cs_disc_padyn_count)
		csv_data["cs discarded padyn pct"] = utils.gen_pct_string(float(cs_disc_padyn_count) / float(cs_disc_padyn_count + cs_acc_count))

		self.log_errors_target(prg_name, ["CS discarded in clang: " + fun[0] + " " + fun[1] + " -> " + str(entry) for (fun, entries) in cs_disc_clang for entry in entries])
		self.log_errors_target(prg_name, ["CS discarded in padyn: " + fun[0] + " " + fun[1] + " -> " + str(entry) for (fun, entries) in cs_disc_padyn for entry in entries])

		self.csv_output_row("matching", csv_data)
