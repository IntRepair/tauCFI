import base_functionality.utilities as utils

def entry_identity(entry):
    return (entry["demang_name"], entry["mang_name"])

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

	acceptable_padyn_clang = [(entry, entry) for entry in padyn_cfns.intersection(clang_cfns)]

	padyn_not_in_clang_map = group_identities_key0(padyn_not_in_clang)
	clang_not_in_padyn_map = group_identities_key0(clang_not_in_padyn)

	for key in list(set(clang_not_in_padyn_map.keys()).intersection(set(padyn_not_in_clang_map.keys()))):
		padyn_identities = padyn_not_in_clang_map[key]
		clang_identities = clang_not_in_padyn_map[key]

		if len(clang_identities) == 1 and len(padyn_identities) == 1:
			acceptable_padyn_clang += [(padyn_identities[0], clang_identities[0])]

			padyn_not_in_clang -= set(padyn_identities)
			clang_not_in_padyn -= set(clang_identities)

	return (acceptable_padyn_clang, padyn_not_in_clang, clang_not_in_padyn)

def build_lookup_padyn(source_key, padyn, X_padyn_clang):
	lookup_padyn = dict([(entry_identity(entry), entry) for entry in padyn[source_key]])

	for delete_key in (set(lookup_padyn.keys()) - set([key_pair[0] for key_pair in X_padyn_clang])):
		del lookup_padyn[delete_key]

	return lookup_padyn

def build_lookup_clang(source_key, clang, X_padyn_clang):
	lookup_clang = dict([(entry_identity(entry), entry) for entry in clang[source_key]])

	for delete_key in (set(lookup_clang.keys()) - set([key_pair[1] for key_pair in X_padyn_clang])):
		del lookup_clang[delete_key]

	return lookup_clang

def address_taken_matching(padyn, clang, cfn_acc_padyn_clang):
	padyn_at_index = set(padyn["at_index"])
	clang_at_index = set(clang["at_index"])

	padyn_at_lookup = build_lookup_padyn("compiled_function", padyn, cfn_acc_padyn_clang)
	clang_at_lookup = build_lookup_clang("compiled_function", clang, cfn_acc_padyn_clang)

	at_acc_padyn_clang = []

	padyn_not_in_clang = []
	clang_not_in_padyn = []

	for key_pair in cfn_acc_padyn_clang:
		padyn_at = key_pair[0] in padyn_at_index
		clang_at = key_pair[1] in padyn_at_index

		if padyn_at and clang_at:
			at_acc_padyn_clang += [(padyn_at_lookup[key_pair[0]], clang_at_lookup[key_pair[1]])]
		elif padyn_at:
			padyn_not_in_clang += [key_pair[0]]
		elif clang_at:
			clang_not_in_padyn += [key_pair[1]]

	return (at_acc_padyn_clang, padyn_not_in_clang, clang_not_in_padyn)

def call_site_matching(padyn, clang, cfn_acc_padyn_clang):
	padyn_css_index = padyn.get("cs_index", {})
	clang_css_index = clang.get("cs_index", {})

	cs_acc_padyn_clang = []

	cs_disc_padyn = []
	cs_disc_clang = []

	for key_pair in cfn_acc_padyn_clang:
		padyn_css = padyn_css_index.get(key_pair[0], [])
		clang_css = clang_css_index.get(key_pair[1], [])

		if len(padyn_css) != 0 or len(clang_css) != 0:
			if len(padyn_css) == len(clang_css):
				cs_acc_padyn_clang += [(key_pair, padyn_css, clang_css)]
			else:
				cs_disc_padyn += [(key_pair[0], padyn_css)]
				cs_disc_clang += [(key_pair[1], clang_css)]

	return (cs_acc_padyn_clang, cs_disc_padyn, cs_disc_clang)
