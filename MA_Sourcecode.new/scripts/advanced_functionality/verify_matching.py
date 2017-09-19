import base_functionality.utilities as utils

def get_functions(data):
    fns = set()
    exclude = data["double_vals"]["ct"].keys()

    for ct in data["call_target"]:
        if ct not in exclude:
            fns.add(ct["origin"])

    return fns

def get_non_plt_functions(data):
    fns = set()
    exclude = data["double_vals"]["ct"].keys()

    for ct in data["call_target"]:
        if ct not in exclude and not ct["plt"]:
            fns.add(ct["origin"])

    return fns


def call_target_matching(padyn, clang):
    padyn_fns = get_functions(padyn)
    clang_fns = get_functions(clang)

    not_in_clang = padyn_fns - clang_fns
    not_in_padyn = clang_fns - padyn_fns

    acceptable = padyn_fns.intersection(clang_fns)

    return (acceptable, not_in_clang, not_in_padyn)

def get_taken_addresses(data):
    ats = set()
    for at in data["address_taken"]:
        ats.add(at["origin"])

    return ats

def address_taken_matching(padyn, clang, fn_acceptable):
    padyn_ats = get_taken_addresses(padyn).intersection(fn_acceptable)
    clang_ats = get_taken_addresses(clang).intersection(fn_acceptable)

    not_in_clang = padyn_ats - clang_ats
    not_in_padyn = clang_ats - padyn_ats

    acceptable = padyn_ats.intersection(clang_ats)

    return (acceptable, not_in_clang, not_in_padyn)

def generate_index(data, fn_acceptable):
    elems = []
    elemI = {}
    for elem in data:
        if elem["origin"] in fn_acceptable:
            utils.add_values_to_key(elemI, elem["origin"], elem)
            elems += [elem]

    return (elems, elemI)

def length(container, key):
    if key not in container.keys():
        return 0

    return len(container[key])

def call_site_matching(padyn, clang, fn_acceptable):
    (padyn_css, padyn_csI) = generate_index(padyn["call_site"], fn_acceptable)
    (clang_css, clang_csI) = generate_index(clang["call_site"], fn_acceptable)

    mismatch = []
    acceptable = []

    for fn in fn_acceptable:
        length_padyn = length(padyn_csI, fn)
        length_clang = length(clang_csI, fn)
        if length_padyn != length_clang:
            mismatch += [(fn, length_padyn, length_clang)]
        elif length_padyn > 0:
            acceptable += [(fn, length_padyn)]

    return (acceptable, mismatch)

def verify(target, padyn, clang):
    prg_name = target.name

    problem_string = ""

    double_vals = clang["double_vals"]

    for key in ["at", "fn", "ct", "cs"]:
        if key in double_vals.keys():
            for entry in double_vals[key]:
                string = "double_vals[" + key + "]: " + entry
                print string
                problem_string += string + "\n"

    csv_data = {}
    csv_data["target"] = prg_name

    (fn_acceptable, fn_not_in_clang, fn_not_in_padyn) = call_target_matching(padyn, clang)
    fn_count = len(fn_acceptable)
    csv_data["fn"] = fn_count
    fn_not_clang = float(len(fn_not_in_clang))
    csv_data["fn not in clang"] = str(int(fn_not_clang))
    csv_data["fn not in clang pct"] = utils.gen_pct_string(fn_not_clang / (fn_not_clang + fn_count))
    fn_not_padyn = float(len(fn_not_in_padyn))
    csv_data["fn not in padyn"] = str(int(fn_not_padyn))
    csv_data["fn not in padyn pct"] = utils.gen_pct_string(fn_not_padyn / (fn_not_padyn + fn_count))

    problem_string += "".join([prg_name + " FN not in clang: " + fun + "\n" for fun in fn_not_in_clang])
    problem_string += "\n"
    problem_string += "".join([prg_name + " FN not in padyn: " + fun + "\n" for fun in fn_not_in_padyn])
    problem_string += "\n"

    # We have a problem when an AT is in clang but not in padyn (after we reduced to fn_acceptable)
    (at_acceptable, at_not_in_clang, at_not_in_padyn) = address_taken_matching(padyn, clang, fn_acceptable)
    at_count = len(at_acceptable)
    csv_data["at"] = at_count
    at_not_clang = float(len(at_not_in_clang))
    csv_data["at not in clang"] = str(int(at_not_clang))
    csv_data["at not in clang pct"] = utils.gen_pct_string(at_not_clang / (at_not_clang + at_count))
    at_not_padyn = float(len(at_not_in_padyn))
    csv_data["at not in padyn"] = str(int(at_not_padyn))
    csv_data["at not in padyn pct"] = utils.gen_pct_string(at_not_padyn / (at_not_padyn + at_count))

    problem_string += "".join([prg_name + " AT not in clang: " + fun + "\n" for fun in at_not_in_clang])
    problem_string += "\n"
    problem_string += "".join([prg_name + " AT not in padyn: " + fun + "\n" for fun in at_not_in_padyn])
    problem_string += "\n"

    (cs_acceptable, cs_mismatch) = call_site_matching(padyn, clang, fn_acceptable)

    cs_count = 0

    for (fn, count) in cs_acceptable:
        cs_count += count

    csv_data["cs"] = cs_count

    cs_clang = 0.0
    cs_padyn = 0.0

    for (fn, length_padyn, length_clang) in cs_mismatch:
        problem_string += prg_name + " CS mismatch " + fn + " padyn " + str(length_padyn) + " clang " + str(length_clang) + "\n"
        cs_clang += float(length_clang)
        cs_padyn += float(length_padyn)

    problem_string += "\n"

    csv_data["cs discarded clang"] = str(int(cs_clang))
    csv_data["cs discarded clang pct"] = utils.gen_pct_string(cs_clang / (cs_clang + cs_count))
    csv_data["cs discarded padyn"] = str(int(cs_padyn))
    csv_data["cs discarded padyn pct"] = utils.gen_pct_string(cs_padyn / (cs_padyn + cs_count))

    return (csv_data, problem_string)
