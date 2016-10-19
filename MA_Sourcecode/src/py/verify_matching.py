import output_parsing as parsing
import utilities as utils

def get_functions(data):
    fns = set()
    for ct in data["call_target"]:
        if "plt" in ct.keys():
            if not bool(ct["plt"]):
                fns.add(ct["origin"])
        else:
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
        if "plt" in at.keys():
            if not bool(at["plt"]):
                ats.add(at["origin"])
        else:
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
            elemI = utils.add_values_to_key(elemI, elem["origin"], elem)
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
        elif length_padyn > 0 :
            acceptable += [(fn, length_padyn)]

    return (acceptable, mismatch)


def verify(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    csv_data = {}
    csv_data["target"] = prg_name

    problem_string = ""
    (fn_acceptable, fn_not_in_clang, fn_not_in_padyn) = call_target_matching(padyn, clang)
    csv_data["fn_count"] = len(fn_acceptable)
    csv_data["fn_problem"] = len(fn_not_in_clang)

    for fn in fn_not_in_clang:
        problem_string += prg_name + " FN not in clang: " + fn + "\n"
    problem_string += "\n"
    for fn in fn_not_in_padyn:
        problem_string += prg_name + " FN not in padyn: " + fn + "\n"
    problem_string += "\n"

    # We have a problem when an AT is in clang but not in padyn (after we reduced to fn_acceptable)
    (at_acceptable, at_not_in_clang, at_not_in_padyn) = address_taken_matching(padyn, clang, fn_acceptable)
    csv_data["at_count"] = len(at_acceptable)
    csv_data["at_problem"] = len(at_not_in_padyn)

    #for at in at_not_in_clang:
    #    problem_string += prg_name + " AT not in clang: " + at + "\n"
    #problem_string += "\n"
    for at in at_not_in_padyn:
        problem_string += prg_name + " AT not in padyn: " + at + "\n"
    problem_string += "\n"

    (cs_acceptable, cs_mismatch) = call_site_matching(padyn, clang, fn_acceptable)

    cs_count = 0

    for (fn, count) in cs_acceptable:
        cs_count += count

    csv_data["cs_count"] = cs_count

    cs_clang = 0
    cs_padyn = 0

    for (fn, length_padyn, length_clang) in cs_mismatch:
        problem_string += prg_name + " CS mismatch " + fn + " padyn " + str(length_padyn) + " clang " + str(length_clang) + "\n"
        cs_clang += length_clang
        cs_padyn += length_padyn

    problem_string += "\n"

    csv_data["cs_clang"] = cs_clang
    csv_data["cs_padyn"] = cs_padyn


    return (csv_data, problem_string)
