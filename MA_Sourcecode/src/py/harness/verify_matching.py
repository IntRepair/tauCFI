import output_parsing as parsing
import utilities as utils

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
        elif length_padyn > 0:
            acceptable += [(fn, length_padyn)]

    return (acceptable, mismatch)

def verify(fldr_path, prg_name):
    print prg_name
    #padyn = parsing.parse_count_prec_verify(fldr_path, prg_name)
    padyn = parsing.parse_verify_basic(fldr_path, prg_name)
    #clang = parsing.parse_ground_truth(fldr_path, prg_name)

    #if prg_name in ["lighttpd", "proftpd"]:
    gt_parser = parsing.GroundTruthParser(fldr_path, prg_name)
    clang = gt_parser.get_binary_info(prg_name)

    #if prg_name == "lighttpd":
    #    gt_parser = parsing.GroundTruthParser(fldr_path, prg_name)
    #    req_name = prg_name
    #    if req_name == "nginx":
    #        req_name = "objs/" + req_name
    #    padyn = gt_parser.get_binary_info(req_name)
        #information = gt_parser.get_binary_info(req_name)
#
    #    print information.keys()
#
#        print "information CT", len(information["call_target"]), len(information["ct_index"]), len(set(information["ct_index"]))
#        print "clang CT", len(clang["call_target"]), len(clang["ct_index"]), len(set(clang["ct_index"]))
#        print "padyn CT", len(padyn["call_target"]), len(padyn["ct_index"]), len(set(padyn["ct_index"]))
#
#        print "information AT", len(information["address_taken"]), len(information["at_index"]), len(set(information["at_index"]))
#        print "clang AT", len(clang["address_taken"]), len(clang["at_index"]), len(set(clang["at_index"]))
#        print "padyn AT", len(padyn["address_taken"]), len(padyn["at_index"]), len(set(padyn["at_index"]))
#
#        print "information FN", len(information["normal_function"]), len(information["fn_index"]), len(set(information["fn_index"]))
#        print "clang FN", len(clang["normal_function"]), len(clang["fn_index"]), len(set(clang["fn_index"]))
#
#        print "information CS", len(information["call_site"])
#        print "clang CS", len(clang["call_site"])
#        print "padyn CS", len(padyn["call_site"])
#
#        padyn = information
#        for cs in padyn["call_site"]:
#            print "padyn ", cs["origin"]
#
#        for cs in information["call_site"]:
#            print "clang ", cs["origin"]

    problem_string = ""

    key_count = (lambda x: len(x.keys()))

    
    double_vals = clang["double_vals"]

    print_string = ""
    print_string = utils.append_key_values(print_string, double_vals, "at")#, key_count)
    print_string = utils.append_key_values(print_string, double_vals, "fn")#, key_count)
    print_string = utils.append_key_values(print_string, double_vals, "ct")#, key_count)
    print_string = utils.append_key_values(print_string, double_vals, "cs")#, key_count)
    print print_string

    #data_gen = (lambda x: ["    " + key + ": " + str(1 + len(x[key])) for key in x.keys()])
    #unfolder = (lambda x: "\n" + "\n".join(data_gen(x)))

    #problem_string = utils.append_key_values(problem_string, double_vals, "at", unfolder)
    #problem_string = utils.append_key_values(problem_string, double_vals, "fn", unfolder)
    #problem_string = utils.append_key_values(problem_string, double_vals, "ct", unfolder)
    #problem_string = utils.append_key_values(problem_string, double_vals, "cs", unfolder)
    #problem_string += "\n"

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
