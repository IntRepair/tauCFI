from advanced_pass import AdvancedPass as AdvancedPass
from parsing.parse_verify import parse_verify as parse_verify_tag

import advanced_functionality.matching as matching
import base_functionality.utilities as utils

def arg_type(entry):
	params = [param["wideness"] for param in entry["params"]]
	if len(params) < 6:
		params += (6 - len(params)) * [0]
	return params[0:6]

def arg_count(entry):
	return min(entry["param_count"], 6)

def count_pairing(key_index, at_acc_padyn_clang, cs_acc_padyn_clang):
	pairing_vals = []

	at_types = []

	for at_pair in at_acc_padyn_clang:
		at_entry = at_pair[key_index]
		at_types += [arg_count(at_entry)]

	# (key_pair, css_padyn, css_clang)
	for cs_triple in cs_acc_padyn_clang:
		css = cs_triple[1 + key_index]
		for (index, cs) in enumerate(css):
			cs_type = arg_count(cs)
			pairing_vals += [len([1 for at_type in at_types if at_type <= cs_type])]

	pairing_vals.sort()
	return pairing_vals

def type_pairing(key_index, at_acc_padyn_clang, cs_acc_padyn_clang):
	pairing_vals = []

	at_types = []

	for at_pair in at_acc_padyn_clang:
		at_entry = at_pair[key_index]
		at_types += [arg_type(at_entry)]

	# (key_pair, css_padyn, css_clang)
	for cs_triple in cs_acc_padyn_clang:
		css = cs_triple[1 + key_index]
		for (index, cs) in enumerate(css):
			cs_type = arg_type(cs)
			pairing_vals += [len([1 for at_type in at_types if at_type <= cs_type])]

	pairing_vals.sort()
	return pairing_vals

class PolicyComparePass(AdvancedPass):
	def __init__(self, config, args):
		AdvancedPass.__init__(self, "policy_compare", config, args)

		fieldnames = ["opt"]
		fieldnames += ["target"]

		fieldnames2 = ["avg", "sig", "median"]
		fieldnames3 = [" safe ", " prec ", "* "]

		fieldnames += ["at"]

		fieldnames += ["count" + fn3 + fn2 for fn3 in fieldnames3 for fn2 in fieldnames2]
		fieldnames += ["type" + fn3 + fn2 for fn3 in fieldnames3 for fn2 in fieldnames2] 

		self.count_ct = config["type_shield.pass.policy_compare.count_ct"] + "count"
		self.count_cs = config["type_shield.pass.policy_compare.count_cs"] + "count"
		self.type_ct = config["type_shield.pass.policy_compare.type_ct"] + "type"
		self.type_cs = config["type_shield.pass.policy_compare.type_cs"] + "type"

		self.csv_set_define("policy_compare", fieldnames)

	def _run_on_target(self, target, padyn, clang):
		prg_name = target.name

		csv_data = {}
		csv_data["opt"] = "O2"
		csv_data["target"] = prg_name

		padyn_count = parse_verify_tag(target.prefix, target.binary_name, self.count_ct, self.count_cs)

		(cfn_acc_padyn_clang_count, _, _) = matching.compiled_function_matching(padyn_count, clang)
		(at_acc_padyn_clang_count, _, _) = matching.address_taken_matching(padyn_count, clang, cfn_acc_padyn_clang_count)
		(cs_acc_padyn_clang_count, _, _) = matching.call_site_matching(padyn_count, clang, cfn_acc_padyn_clang_count)

		csv_data["at"] = len(at_acc_padyn_clang_count)

		csv_data["count safe avg"] = "0"
		csv_data["count safe sig"] = "0"
		csv_data["count safe median"] = "0"

		count_pairing_vals = count_pairing(0, at_acc_padyn_clang_count, cs_acc_padyn_clang_count)

		csv_data["count prec avg"] = utils.gen_pct_string(utils.expected_value(count_pairing_vals) / float(100))
		csv_data["count prec sig"] = utils.gen_pct_string(utils.std_deviation(count_pairing_vals) / float(100))
		csv_data["count prec median"] = utils.median(count_pairing_vals)

		perfect_count_pairing_vals = count_pairing(1, at_acc_padyn_clang_count, cs_acc_padyn_clang_count)

		csv_data["count* avg"] = utils.gen_pct_string(utils.expected_value(perfect_count_pairing_vals) / float(100))
		csv_data["count* sig"] = utils.gen_pct_string(utils.std_deviation(perfect_count_pairing_vals) / float(100))
		csv_data["count* median"] = utils.median(perfect_count_pairing_vals)

		csv_data["type safe avg"] = "0"
		csv_data["type safe sig"] = "0"
		csv_data["type safe median"] = "0"

		padyn_type = parse_verify_tag(target.prefix, target.binary_name, self.type_ct, self.type_cs)

		(cfn_acc_padyn_clang_type, _, _) = matching.compiled_function_matching(padyn_type, clang)
		(at_acc_padyn_clang_type, _, _) = matching.address_taken_matching(padyn_type, clang, cfn_acc_padyn_clang_type)
		(cs_acc_padyn_clang_type, _, _) = matching.call_site_matching(padyn_type, clang, cfn_acc_padyn_clang_type)

		type_pairing_vals = type_pairing(0, at_acc_padyn_clang_type, cs_acc_padyn_clang_type)

		csv_data["type prec avg"] = utils.gen_pct_string(utils.expected_value(type_pairing_vals) / float(100))
		csv_data["type prec sig"] = utils.gen_pct_string(utils.std_deviation(type_pairing_vals) / float(100))
		csv_data["type prec median"] = utils.median(type_pairing_vals)

		perfect_type_pairing_vals = type_pairing(1, at_acc_padyn_clang_type, cs_acc_padyn_clang_type)

		csv_data["type* avg"] = utils.gen_pct_string(utils.expected_value(perfect_type_pairing_vals) / float(100))
		csv_data["type* sig"] = utils.gen_pct_string(utils.std_deviation(perfect_type_pairing_vals) / float(100))
		csv_data["type* median"] = utils.median(perfect_type_pairing_vals)

		self.csv_output_row("policy_compare", csv_data)

#opt,target,at,count safe avg,count safe sig,count safe median,count prec avg,count prec sig,count prec median,count* avg,count* sig,count* #median,type safe avg,type safe sig,type safe median,type prec avg,type prec sig,type prec median,type* avg,type* sig,type* median
#O2,ProFTPD,396,,,,334.5,51.26,311.0,330.31,48.07,343.0,,,,337.41,54.09,336.0,310.58,60.33,323.0
#O2,Pure-FTPD,13,,,,9.87,4.32,13.0,5.5,4.82,6.5,,,,8.12,4.11,7.0,4.37,4.92,2.0
#O2,Vsftpd,10,,,,7.85,1.39,7.0,7.14,1.81,6.0,,,,6.42,0.96,7.0,5.42,0.95,6.0
#O2,Lighttpd,63,,,,41.19,13.22,41.0,27.75,10.73,24.0,,,,41.42,14.29,38.0,25.1,8.98,24.0
#O2,MySQL,5896,,,,4281.71,1267.78,4403.0,2804.69,1064.83,2725.0,,,,3617.51,1390.09,3792.0,2043.58,1091.05,1564.0
#O2,Postgressql,2504,,,,1990.59,574.53,2122.0,1964.83,618.28,2124.0,,,,1624.07,707.58,1786.0,1747.22,727.08,2004.0
#O2,Memcached,14,,,,12.0,1.38,13.0,11.91,2.84,14.0,,,,10.25,0.77,10.0,9.97,1.45,11.0
#O2,Node.js,7230,,,,5306.05,1694.73,5429.0,3406.07,1666.9,2705.0,,,,4229.22,2038.64,3864.0,2270.28,1720.32,1707.0
#O2,geomean,216.61,,,,166.09,40.28,171.97,129.77,43.99,127.62,,,,144.06,38.38,141.82,105.13,38.68,92.74
 
