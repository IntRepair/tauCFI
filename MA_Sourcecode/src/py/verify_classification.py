import harness.output_parsing as parsing
import harness.utilities as utils

#def compare_address_taken(ground_truth, verify):
#    print "Comparing the set of address taken functions:"
#    gt_index = []
#    v_index = []
#
#    for elem in verify["address_taken"]:
#        v_index += [elem["origin"]]
#
#    for elem in ground_truth["address_taken"]:
#        gt_index += [elem["origin"]]
#
#    missing_fns = set(gt_index) - set(v_index)
#    more_fns = set(v_index) - set(gt_index)
#
#    print "    ", len(missing_fns),
#    print "address taken functions are missing compared to clang ground truth"
#    print "    ", len(more_fns),
#    print "more address taken functions are found compared to clang ground truth"
#    print
#
#def compare_call_target(ground_truth, verify):
#    print "Comparing the set of call targets:"
#    gt_index = {}
#
#    for elem in ground_truth["address_taken"]:
#        gt_index[elem["origin"]] = elem
#
#    for elem in ground_truth["normal_function"]:
#        gt_index[elem["origin"]] = elem
#
#    print "    ", "Looking at parameter counts"
#
#    not_acceptable = 0
#    acceptable = 0
#    exact_match = 0
#
#    distance = []
#
#    for v_elem in verify["call_target"]:
#        key = v_elem["origin"]
#        process_key = key in gt_index.keys()
#        if not process_key:
#            print "    ", "    ", "ground_truth does not contain", key
#        else:
#            gt_elem = gt_index[key]
#
#            v_count = int(v_elem["param_count"])
#            gt_count = int(gt_elem["param_count"])
#
#            if v_count > gt_count:
#                print "    ", "    ", "more params (not acceptable) in verify than in ground truth for", key
#                print v_elem
#                print gt_elem
#                not_acceptable += 1
#
#            if v_count == gt_count:
#                #print "    ", "    ", "exact param count match verify vs ground truth for", key
#                exact_match += 1
#
#            if v_count <= gt_count:
#                #print "    ", "    ", "less params (acceptable) in verify than in ground truth for", key
#                acceptable += 1
#                distance += [gt_count - v_count]
#
#    print "    ", "    ", "not acceptable functions:", not_acceptable
#    print "    ", "    ", "acceptable functions:", acceptable
#    print "    ", "    ", "exact matches:", exact_match
#
#    for i in range(6):
#        print "    ", "    ", "distance      :", i, distance.count(i)
#
#    print "    ", "    ", "distance (avg):", float(sum(distance)) / float(len(distance))
#    print
#
#    print "    ", "Looking at the voidness of return types"
#
#    exact_match = 0
#    acceptable = 0
#    mismatch = 0
#
#    for v_elem in verify["call_target"]:
#        key = v_elem["origin"]
#        process_key = key in gt_index.keys()
#        if not process_key:
#            print "    ", "    ", "ground_truth does not contain", key
#        else:
#            gt_elem = gt_index[key]
#
#            v_void = (v_elem["return_type"] == "0")
#            gt_void = gt_elem["return_type"]["is_void"]
#
#            if not gt_void and v_void:
#                print "    ", "    ",
#                print "return type mismatch (possibly problematic) verify vs ground truth for",
#                print key
#                print "    ", "    ", v_elem
#                print "    ", "    ", gt_elem
#                mismatch += 1
#            else:
#                if v_void == gt_void:
#                    #print "    ", "exact return type match verify vs ground truth for", key
#                    exact_match += 1
#                else:
#                    #print "    ", "return type mismatch(acceptable) verify vs ground truth for", key
#                    acceptable += 1
#
#    print "    ", "    ", "possibly problematic mismatches:", mismatch
#    print "    ", "    ", "acceptable mismatches:", acceptable
#    print "    ", "    ", "exact matches:", exact_match
#    print
#
#def callsites_return_mergeable(gt_sites):
#    gt_site = gt_sites[0]
#    for site in gt_sites:
#        if site["return_type"]["is_void"] != gt_site["return_type"]["is_void"]:
#            return False
#    return True
#
#def callsites_param_mergeable(gt_sites):
#    gt_site = gt_sites[0]
#    for site in gt_sites:
#        if site["param_count"] != gt_site["param_count"]:
#            return False
#    return True
#
#def compare_call_site(ground_truth, verify):
#    print "Comparing the set of call sites:"
#    gt_index = {}
#    v_index = {}
#    for elem in ground_truth["call_site"]:
#        utils.add_values_to_key(gt_index, elem["origin"], elem)
#
#    for elem in verify["call_site"]:
#        utils.add_values_to_key(v_index, elem["origin"], elem)
#
#    print "    ", "Looking at differences between the number of call sites between ground truth and verify"
#    print "    ", "    ", "GT", len(ground_truth["call_site"]), len(gt_index.keys())
#    print "    ", "    ", "V ", len(verify["call_site"]), len(v_index.keys())
#    print "    ", "    ", "GT - V", set(gt_index.keys()) - set(v_index.keys())
#    print "    ", "    ", "V - GT", set(v_index.keys()) - set(gt_index.keys())
#    print
#
#    print "    ", "Looking at parameter counts"
#
#    not_acceptable = 0
#    acceptable = 0
#    exact_match = 0
#
#    distance = []
#
#    mismatches = []
#
#    for key in v_index.keys():
#        process_key = key in gt_index.keys()
#        if not process_key:
#            print "    ", "    ", "ground_truth does not contain", key
#        else:
#            v_sites = v_index[key]
#            gt_sites = gt_index[key]
#            if len(v_sites) != len(gt_sites) and not callsites_param_mergeable(gt_sites):
#                mismatches += [(key, len(gt_sites), len(v_sites))]
#            else:
#                if callsites_param_mergeable(gt_sites):
#                    gt_sites = [gt_sites[0]] * len(v_sites)
#
#                for index, (v_elem, gt_elem) in enumerate(zip(v_sites, gt_sites)):
#                    v_count = int(v_elem["param_count"])
#                    gt_count = int(gt_elem["param_count"])
#                    if v_count < gt_count:
#                        print "    ", "    ", "less params (not acceptable) in verify than in ground truth for", index, key
#                        print "    ", "    ", v_elem
#                        print "    ", "    ", gt_elem
#                        print
#                        not_acceptable += 1
#
#                    if v_count == gt_count:
#                        #print "    ", "    ", "exact param count match verify vs ground truth for", key
#                        exact_match += 1
#
#                    if v_count >= gt_count:
##                        if v_count - gt_count > 2:
##                            print "    ", "    ", "more params (acceptable) in verify than in ground truth for", key
##                            print "    ", "    ", v_elem
##                            print "    ", "    ", gt_elem
#
#                        acceptable += 1
#                        distance += [v_count - gt_count]
#
#    print "    ", "    ", "not acceptable functions:", not_acceptable
#    print "    ", "    ", "acceptable functions:", acceptable
#    print "    ", "    ", "exact matches:", exact_match
#
#    for i in range(6):
#        print "    ", "    ", "distance      :", i, distance.count(i)
#
#    print "    ", "    ", "distance (avg):", float(sum(distance)) / float(len(distance))
#    print
#
#    for key, len_gt, len_v in mismatches:
#        print "    ", "    ", "Callsite count missmatch", key, len_gt, "[GT]", len_v, "[V]"
#        v_sites = v_index[key]
#        gt_sites = gt_index[key]
#
#        for site in v_sites:
#            print "    ", "    ", site["param_count"], site["params"]
#
#        print
#
#        for site in gt_sites:
#            print "    ", "    ", site["param_count"], site["params"]
#
#        print
#        print
#
#
#    print
#    print "    ", "Looking at the voidness of return types"
#
#    exact_match = 0
#    acceptable = 0
#    mismatch = 0
#    mismatches = []
#
#    for key in v_index.keys():
#        process_key = key in gt_index.keys()
#        if not process_key:
#            print "    ", "    ", "ground_truth does not contain", key
#        else:
#            v_sites = v_index[key]
#            gt_sites = gt_index[key]
#            if len(v_sites) != len(gt_sites) and not callsites_return_mergeable(gt_sites):
#                mismatches += [(key, len(gt_sites), len(v_sites))]
#            else:
#                if callsites_return_mergeable(gt_sites):
#                    gt_sites = [gt_sites[0]] * len(v_sites)
#
#                for index, (v_elem, gt_elem) in enumerate(zip(v_sites, gt_sites)):
#                    v_void = (v_elem["return_type"] == "0")
#                    gt_void = gt_elem["return_type"]["is_void"]
#
#                    if not v_void and gt_void:
#                        print "    ", "    ",
#                        print "return type mismatch (possibly problematic) verify vs ground truth for",
#                        print "    ", "    ", key
#                        print "    ", "    ", v_elem
#                        print "    ", "    ", gt_elem
#                        print
#                        mismatch += 1
#                    else:
#                        if v_void == gt_void:
#                            #print "    ", "    ", "exact return type match verify vs ground truth for", key
#                            exact_match += 1
#                        else:
#                            #print "    ", "    ", "return type mismatch(acceptable) verify vs ground truth for", key
#                            acceptable += 1
#
#    print "    ", "    ", "possibly problematic mismatches:", mismatch
#    print "    ", "    ", "acceptable mismatches:", acceptable
#    print "    ", "    ", "exact matches:", exact_match
#
#    for key, len_gt, len_v in mismatches:
#        print "    ", "    ", "Callsite count missmatch", key, len_gt, "[GT]", len_v, "[V]"


import harness.verify_matching as verify_matching

clang_global = {}

def get_acceptable_functions(padyn, clang):
    padyn_fns = verify_matching.get_non_plt_functions(padyn)
    clang_fns = verify_matching.get_functions(clang)

    return padyn_fns.intersection(clang_fns)

def get_calltarget_index(data, fn_acceptable):
    ctI = {}
    for elem in data["call_target"]:
        if elem["origin"] in fn_acceptable:
            ctI = utils.add_values_to_key(ctI, elem["origin"], elem)

    return ctI

def get_callsite_infos(padyn, clang, fn_acceptable):
    (padyn_css, padyn_csI) = verify_matching.generate_index(padyn["call_site"], fn_acceptable)
    (clang_css, clang_csI) = verify_matching.generate_index(clang["call_site"], fn_acceptable)

    acceptable = []

    for fn in fn_acceptable:
        length_padyn = verify_matching.length(padyn_csI, fn)
        length_clang = verify_matching.length(clang_csI, fn)
        if length_padyn == length_clang and length_padyn > 0:
            acceptable += [fn]

    return (acceptable, padyn_csI, clang_csI)

def __verify_count(fldr_path, prg_name, padyn, clang):
    csv_data = {}
    csv_data["target"] = prg_name

    problem_string = ""

    ct_problems = 0
    ct_count = 0
    ct_result = [[] for i in range(7)]

    ct_void_ok = 0
    ct_void_problem = 0

    acceptable_fns = get_acceptable_functions(padyn, clang)

    padyn_ctI = get_calltarget_index(padyn, acceptable_fns)
    clang_ctI = get_calltarget_index(clang, acceptable_fns)

    for fn in acceptable_fns:
        padyn_cts = padyn_ctI[fn]
        clang_cts = clang_ctI[fn]

        for (padyn_ct, clang_ct) in zip(padyn_cts, clang_cts):
            dist = int(clang_ct["param_count"]) - int(padyn_ct["param_count"])
            ct_count += 1
            if dist < 0:
                ct_problems += 1
                problem_string += prg_name + " CT mismatch: " + fn + " clang: "+ str(clang_ct["param_count"]) + " padyn: " + str(padyn_ct["param_count"]) + "\n"
            elif dist < 6:
                ct_result[dist] += [fn]
                if dist != 0:
                    problem_string += prg_name + " CT accept: " + fn + " clang: "+ str(clang_ct["param_count"]) + " padyn: " + str(padyn_ct["param_count"]) + "\n"

            else:
                ct_result[6] += [fn]

            padyn_void = bool(padyn_ct["return_type"]["is_void"])
            clang_void = bool(clang_ct["return_type"]["is_void"])

            if padyn_void == clang_void:
                ct_void_ok += 1
            elif padyn_void and (not clang_void):
                ct_void_problem += 1
                problem_string += prg_name + " CT void problem: " + fn + " clang: non-void padyn: void\n"

    ct_dict = {}
    ct_dict["target"] = prg_name
    ct_dict["ct"] = ct_count
    ct_dict["problems"] = ct_problems
    ct_dict["void-ok"] = ct_void_ok
    ct_dict["void-problem"] = ct_void_problem

    for index in range(7):
        ct_dict["-" + str(index)] = len(ct_result[index])

    problem_string += "\n"

    cs_problems = 0
    cs_count = 0
    cs_result = [[] for i in range(7)]

    cs_non_void_ok = 0
    cs_non_void_problem = 0

    (acceptable_callsites, padyn_csI, clang_csI) = get_callsite_infos(padyn, clang, acceptable_fns)

    for cs_fn in acceptable_callsites:
        padyn_css = padyn_csI[cs_fn]
        clang_css = clang_csI[cs_fn]

        index = 0
        for (padyn_cs, clang_cs) in zip(padyn_css, clang_css):
            clang_param_count = int(clang_cs["param_count"])
            dist = int(padyn_cs["param_count"]) - clang_param_count
            cs_count += 1
            if dist < 0 and (clang_param_count <= 6):
                cs_problems += 1
                problem_string += prg_name + " CS mismatch: " + cs_fn + " [" + str(index) + "] clang: " + str(clang_param_count) + "[real: "+ str(clang_cs["param_count"]) + "] padyn: " + str(padyn_cs["param_count"]) + "\n"
            elif dist < 0 and int(padyn_cs["param_count"]) < 6:
                print prg_name + " CS mismatch: " + cs_fn + " [" + str(index) + "] clang: " + str(clang_param_count) + "[real: "+ str(clang_cs["param_count"]) + "] padyn: " + str(padyn_cs["param_count"]) + "\n"
            elif dist < 6:
                if dist != 0:
                    problem_string += prg_name + " CS accept: " + cs_fn + " [" + str(index) + "] clang: "+ str(clang_cs["param_count"]) + " padyn: " + str(padyn_cs["param_count"]) + "\n"

                # HACKFIX FOR NOW
                if dist < 0:
                    dist = 0
                cs_result[dist] += [cs_fn]
            else:
                problem_string += prg_name + " CS accept: " + cs_fn + " [" + str(index) + "] clang: "+ str(clang_cs["param_count"]) + " padyn: " + str(padyn_cs["param_count"]) + "\n"
                cs_result[6] += [cs_fn]
            index += 1

            padyn_non_void = not bool(padyn_cs["return_type"]["is_void"])
            clang_non_void = not bool(clang_cs["return_type"]["is_void"])

            if padyn_non_void == clang_non_void:
                cs_non_void_ok += 1
            elif padyn_non_void and (not clang_non_void):
                cs_non_void_problem += 1
                problem_string += prg_name + " CS void problem: " + cs_fn + " [" + str(index) + "] clang: void padyn: non-void\n"

    cs_dict = {}
    cs_dict["target"] = prg_name
    cs_dict["cs"] = cs_count
    cs_dict["problems"] = cs_problems
    cs_dict["non-void-ok"] = cs_non_void_ok
    cs_dict["non-void-problem"] = cs_non_void_problem

    for index in range(7):
        cs_dict["+" + str(index)] = len(cs_result[index])

    return (cs_dict, ct_dict, problem_string)

def bucket_wideness(wideness):
    val = int(wideness)

    if val == 0:
        return 0
    elif val <= 8:
        return 8
    elif val <= 16:
        return 16
    elif val <= 32:
        return 32
    else:
        return 64

def __param_to_string(data):
    count = int(data["param_count"])

    bucket_id = str(count) + " "

    for param in data["params"]:
        bucket_id += str(param["wideness"]).strip() + " "

    bucket_id += "0" #str(data["return_type"]["wideness"])

    return bucket_id

def __verify_type(fldr_path, prg_name, padyn, clang):
    csv_data = {}
    csv_data["target"] = prg_name

    problem_string = ""
    problem_info = ""

    ct_problems = 0
    ct_count = 0
    ct_perfect = 0

    ct_void_ok = 0
    ct_void_problem = 0

    acceptable_fns = get_acceptable_functions(padyn, clang)

    padyn_ctI = get_calltarget_index(padyn, acceptable_fns)
    clang_ctI = get_calltarget_index(clang, acceptable_fns)

    acceptable_buckets = {}
    problematic_buckets = {}

    for fn in acceptable_fns:
        padyn_cts = padyn_ctI[fn]
        clang_cts = clang_ctI[fn]

        for (padyn_ct, clang_ct) in zip(padyn_cts, clang_cts):

            dist = int(clang_ct["param_count"]) - int(padyn_ct["param_count"])
            ct_count += 1
            if dist < 0:
                ct_problems += 1
                problem_string += prg_name + " CT mismatch: " + fn + " clang: "+ str(clang_ct["param_count"]) + " padyn: " + str(padyn_ct["param_count"]) + "\n"
                utils.add_values_to_key(problematic_buckets, __param_to_string(clang_ct), __param_to_string(padyn_ct))
            else:
                perfect = bool(dist == 0)
                problem = False
                problem_info = ""
                param_count = 0
                for (param_clang, param_padyn) in zip(clang_ct["params"], padyn_ct["params"]):
                    if bucket_wideness(param_clang["wideness"]) < bucket_wideness(param_padyn["wideness"]):
                        problem = True
                        perfect = False
                        problem_info += "param no " + str(param_count) + " " + str(bucket_wideness(param_clang["wideness"])) + " " + str(bucket_wideness(param_padyn["wideness"]))
                    elif bucket_wideness(param_clang["wideness"]) > bucket_wideness(param_padyn["wideness"]):
                        perfect = False
                    param_count += 1

                if problem:
                    ct_problems += 1
                    problem_string += prg_name + " CT mismatch [" + str(clang_ct["param_count"]) + "-" + str(padyn_ct["param_count"]) + "] { " + problem_info + "}: " + fn + " clang: "+ str(clang_ct["params"]) + " padyn: " + str(padyn_ct["params"]) + "\n"
                    utils.add_values_to_key(problematic_buckets, __param_to_string(clang_ct), __param_to_string(padyn_ct))
                elif perfect:
                    ct_perfect += 1
                    utils.add_values_to_key(acceptable_buckets, __param_to_string(clang_ct), __param_to_string(padyn_ct))
                else:
                    utils.add_values_to_key(acceptable_buckets, __param_to_string(clang_ct), __param_to_string(padyn_ct))

            padyn_void = bool(padyn_ct["return_type"]["is_void"])
            clang_void = bool(clang_ct["return_type"]["is_void"])

            if padyn_void == clang_void:
                ct_void_ok += 1
            elif padyn_void and (not clang_void):
                ct_void_problem += 1
                problem_string += prg_name + " CT void problem: " + fn + " clang: non-void padyn: void\n"

    problem_string += "\n"

    for key in acceptable_buckets.keys():
        problem_string += "acceptable_matching: " + key + " -> " + str(len(acceptable_buckets[key])) + "\n"

    problem_string += "\n"

    for key in problematic_buckets.keys():
        for value in problematic_buckets[key]:
            problem_string += "problematic_matching: " + key + " -> " + value + "\n"

    problem_string += "\n"
    problem_string += "\n"

    ct_dict = {}
    ct_dict["target"] = prg_name
    ct_dict["ct"] = ct_count
    ct_dict["perfect"] = ct_perfect
    ct_dict["problems"] = ct_problems
    ct_dict["void-ok"] = ct_void_ok
    ct_dict["void-problem"] = ct_void_problem

    problem_string += "\n"

    cs_problems = 0
    cs_count = 0
    cs_perfect = 0

    cs_non_void_ok = 0
    cs_non_void_problem = 0

    (acceptable_callsites, padyn_csI, clang_csI) = get_callsite_infos(padyn, clang, acceptable_fns)

    for cs_fn in acceptable_callsites:
        padyn_css = padyn_csI[cs_fn]
        clang_css = clang_csI[cs_fn]

        index = 0
        for (padyn_cs, clang_cs) in zip(padyn_css, clang_css):
            clang_param_count = int(clang_cs["param_count"])
            dist = int(padyn_cs["param_count"]) - clang_param_count
            cs_count += 1
            if dist < 0 and clang_param_count <= 6:
                cs_problems += 1
                problem_string += prg_name + " CS mismatch: " + cs_fn + " [" + str(index) + "] clang: "+ str(clang_param_count) + "[real:" + str(clang_cs["param_count"]) + "] padyn: " + str(padyn_cs["param_count"]) + "\n"
            else:
                perfect = bool(dist == 0)
                cs_info = ""
                problem = False
                param_count = 0
                for (param_clang, param_padyn) in zip(clang_cs["params"], padyn_cs["params"]):
                    if bucket_wideness(param_clang["wideness"]) > bucket_wideness(param_padyn["wideness"]):
                        cs_info += "param no " + str(param_count) + " CLANG " + str(bucket_wideness(param_clang["wideness"])) + " PADYN " + str(bucket_wideness(param_padyn["wideness"]))
                        problem = True
                        perfect = False
                    elif bucket_wideness(param_clang["wideness"]) < bucket_wideness(param_padyn["wideness"]):
                        cs_info += "param no " + str(param_count) + " CLANG " + str(bucket_wideness(param_clang["wideness"])) + " PADYN " + str(bucket_wideness(param_padyn["wideness"]))
                        perfect = False
                    param_count += 1
                if problem:
                    cs_problems += 1
                    problem_string += prg_name + " CS mismatch: " + cs_fn + " [" + str(index) + "] {" + cs_info + "} clang: "+ str(clang_cs["params"]) + " padyn: " + str(padyn_cs["params"]) + "\n"
                elif perfect:
                    cs_perfect += 1
                else:
                    problem_string += prg_name + " CS acceptable: " + cs_fn + " [" + str(index) + "] {" + cs_info + "} clang: "+ str(clang_cs["params"]) + " padyn: " + str(padyn_cs["params"]) + "\n"

            index += 1

            padyn_non_void = not bool(padyn_cs["return_type"]["is_void"])
            clang_non_void = not bool(clang_cs["return_type"]["is_void"])

            if padyn_non_void == clang_non_void:
                cs_non_void_ok += 1
            elif padyn_non_void and (not clang_non_void):
                cs_non_void_problem += 1
                problem_string += prg_name + " CS void problem: " + cs_fn + " [" + str(index) + "] clang: void padyn: non-void\n"

    cs_dict = {}
    cs_dict["target"] = prg_name
    cs_dict["cs"] = cs_count
    cs_dict["perfect"] = cs_perfect
    cs_dict["problems"] = cs_problems
    cs_dict["non-void-ok"] = cs_non_void_ok
    cs_dict["non-void-problem"] = cs_non_void_problem

    return (cs_dict, ct_dict, problem_string)

def verify(fldr_path, prg_name):
    padyn = parsing.parse_count_prec_verify(fldr_path, prg_name)
    clang = parsing.parse_ground_truth(fldr_path, prg_name)

#    padyn = parsing.parse_count_prec_verify(fldr_path, prg_name)
#    clang = parsing.parse_ground_truth(fldr_path, prg_name)
    return __verify_count(fldr_path, prg_name, padyn, clang)

def verify_ext(fldr_path, prg_name, tag):
    padyn = parsing.parse_verify(fldr_path, prg_name, tag)

    if prg_name not in clang_global.keys():
        gt_parser = parsing.GroundTruthParser(fldr_path, prg_name)
        clang_global[prg_name] = gt_parser.get_binary_info(prg_name)

    clang = clang_global[prg_name]
    #padyn = parsing.parse_count_prec_verify(fldr_path, tag + "." + prg_name)
    #clang = parsing.parse_ground_truth(fldr_path, prg_name)

    return __verify_count(fldr_path, prg_name, padyn, clang)

def verify_type_ext(fldr_path, prg_name, tag):
    padyn = parsing.parse_verify(fldr_path, prg_name, tag)

    if prg_name not in clang_global.keys():
        gt_parser = parsing.GroundTruthParser(fldr_path, prg_name)
        clang_global[prg_name] = gt_parser.get_binary_info(prg_name)

    clang = clang_global[prg_name]
    #padyn = parsing.parse_count_prec_verify(fldr_path, tag + "" + prg_name)
    #clang = parsing.parse_ground_truth(fldr_path, prg_name)

    return __verify_type(fldr_path, prg_name, padyn, clang)

def verify_type(fldr_path, prg_name):
    padyn = parsing.parse_count_prec_verify(fldr_path, "type." + prg_name)
    clang = parsing.parse_ground_truth(fldr_path, prg_name)

    return __verify_type(fldr_path, prg_name, padyn, clang)

def verify_type_count(fldr_path, prg_name):
    padyn = parsing.parse_count_prec_verify(fldr_path, "type." + prg_name)
    clang = parsing.parse_ground_truth(fldr_path, prg_name)

    return __verify_count(fldr_path, prg_name, padyn, clang)
