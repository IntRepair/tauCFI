import sys
import os


#ngx_atoi;               <FN>;   i8*;                    i64; ; ; ; ; parameter_count 2;return_type i64;
#ngx_sort;               <CS>;   i8*;                    i8*; ; ; ; ; parameter_count 2;return_type i64;
#ngx_clean_old_cycles;   <AT>;   %struct.ngx_event_s*;   ;    ; ; ; ; parameter_count 1;return_type void;

def __append_or_set_if_not_exist(dictionary, key, value):
    if key in dictionary.keys():
        dictionary[key] += value
    else:
        dictionary[key] = value
    return dictionary

def __decode_type(type_string):
    type_string = type_string.strip()

    type_info = {}
    type_info["raw"] = type_string
    type_info["is_empty"] = (type_string == "")
    type_info["is_void"] = (type_string == "void")
    if "*" in type_string:
        type_info["is_pointer"] = True
        type_info["wideness"] = 64
    elif not type_info["is_empty"]:
        type_info["is_pointer"] = False
        type_info["usable"] = True
        if type_string == "u64" or type_string == "i64":
            type_info["wideness"] = 64
        elif type_string == "u32" or type_string == "i32":
            type_info["wideness"] = 32
        elif type_string == "u16" or type_string == "i16":
            type_info["wideness"] = 16
        elif type_string == "u8" or type_string == "i8":
            type_info["wideness"] = 8
        elif type_info["is_void"]:
            type_info["wideness"] = 0
        else:
            type_info["not_usable"] = False

    return type_info

def acceptable_site_target(site, target, policy):
    policy = policy.lower()
    if policy == "at":
        return True
    elif policy == "param_count":
        if int(site["param_count"]) < int(target["param_count"]):
            return False
    elif policy == "param_count*":
        if not acceptable_site_target(site, target, "param_count"):
            return False

        if (not site["return_type"]["is_void"]) and (target["return_type"]["is_void"]):
            return False
    elif policy == "param_type":
        if not acceptable_site_target(site, target, "param_count*"):
            return False

        for index in range(int(target["param_count"])):
            if site["params"][index]["wideness"] < target["params"][index]["wideness"]:
                return False

        if site["return_type"]["wideness"] > target["return_type"]["wideness"]:
            return False

    elif policy == "exact":
        if int(site["param_count"]) != int(target["param_count"]):
            return False

        if site["return_type"] != target["return_type"]:
            return False

        for index in range(int(target["param_count"])):
            if site["params"][index] != target["params"][index]:
                return False
    else:
        print "Unknown Policy"
        exit(-1)
    return (site, target, policy, "at")

def __signatures_match_exactly(site, target):
    if int(site["param_count"]) != int(target["param_count"]):
        return False

    if site["return_type"] != target["return_type"]:
        return False

#    for index in range(int(target["param_count"])):
#        if site["params"][index] != target["params"][index]:
#            return False
    return True

def __map_line_to_values(line):
    values = {}
    params = []


    line = line.strip()
    tokens = line.split(";")

    values["param_count"] = tokens[-3].strip().split(" ")[1]
    values["return_type"] = __decode_type(tokens[-2].split(" ")[1])

    for index, token in enumerate(tokens):
        token = token.strip()
        if index == 0:
            values["origin"] = token
#        elif index == 1:
#            values["tag"] = token
        elif index > 1 and (index - 2) < values["param_count"] :
            params += [__decode_type(token)]

    values["params"] = params
    return values

def __map_line_to_values_at_verify(line):
    values = {}
    for index, token in enumerate(line.split(" ")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = token
        if index == 2:
            values["address"] = token
    return values

def __map_line_to_values_cs_ct_verify(line):
    values = {}
    params = []
    param_count = 0

    for index, token in enumerate(line.split(" ")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = token
        if index == 2:
            values["address"] = token
        elif index > 2 and index <= 8:
            params += [token]
            if token == 'x':
                param_count += 1
        elif index == 9:
            values["return_type"] = token
        elif index == 10:
            values["block_start"] = token
        elif index == 11:
            values["instr_addr"] = token

    values["param_count"] = param_count
    values["params"] = params
    return values

def __add_values_to_key(result, key, values):
    result = __append_or_set_if_not_exist(result, key, [values])
    return result

def __analyze_file_line_ground_truth(result, line):
    if line.count("<AT>") > 0:
        result = __add_values_to_key(result, "address_taken", __map_line_to_values(line))
    elif line.count("<FN>") > 0:
        result = __add_values_to_key(result, "normal_function", __map_line_to_values(line))
    elif line.count("<CS>") > 0:
        result = __add_values_to_key(result, "call_site", __map_line_to_values(line))

    return result

def __analyze_file_line_verify(result, line):
    if line.count("<AT>") > 0:
        result = __add_values_to_key(result, "address_taken", __map_line_to_values_at_verify(line))
    elif line.count("<CT>") > 0:
        result = __add_values_to_key(result, "call_target", __map_line_to_values_cs_ct_verify(line))
    elif line.count("<CS>") > 0:
        result = __add_values_to_key(result, "call_site", __map_line_to_values_cs_ct_verify(line))

    return result

def __analyze_file_lines(in_file, analyzer, result={}):
    for line in in_file:
        result = analyzer(result, line)

    return result

def analyze_file_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_ground_truth)

def analyze_file_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify." + name + ".at")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".cs")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".ct")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    return result

def compare_address_taken(ground_truth, verify):
    print "Comparing the set of address taken functions:"
    gt_index = []
    v_index = []

    for elem in verify["address_taken"]:
        v_index += [elem["origin"]]

    for elem in ground_truth["address_taken"]:
        gt_index += [elem["origin"]]

    missing_fns = set(gt_index) - set(v_index)
    more_fns = set(v_index) - set(gt_index)

    print "    ", len(missing_fns),
    print "address taken functions are missing compared to clang ground truth"
    print "    ", len(more_fns),
    print "more address taken functions are found compared to clang ground truth"

def compare_call_target(ground_truth, verify):
    print "Comparing the set of call targets:"
    gt_index = {}

    for elem in ground_truth["address_taken"]:
        gt_index[elem["origin"]] = elem

    for elem in ground_truth["normal_function"]:
        gt_index[elem["origin"]] = elem

    not_acceptable = 0
    acceptable = 0
    exact_match = 0

    distance = []

    for v_elem in verify["call_target"]:
        key = v_elem["origin"]
        process_key = False
        if key != "__libc_csu_init" and key != "__libc_csu_fini" and key != "_init" and key != "_start" and key != "_fini":
            process_key = key in gt_index.keys()
            if not process_key:
                print "ground_truth does not contain", key

        if process_key:
            gt_elem = gt_index[key]

            v_count = int(v_elem["param_count"])
            gt_count = int(gt_elem["param_count"])

            if v_count > gt_count:
                print "    ", "more params (not acceptable) in verify than in ground truth for", key
                print v_elem
                print gt_elem
                not_acceptable += 1

            if v_count == gt_count:
                #print "    ", "exact param count match verify vs ground truth for", key
                exact_match += 1

            if v_count <= gt_count:
                #print "    ", "less params (acceptable) in verify than in ground truth for", key
                acceptable += 1
                distance += [gt_count - v_count]

    print "    ", "not acceptable functions:", not_acceptable
    print "    ", "acceptable functions:", acceptable
    print "    ", "exact matches:", exact_match

    for i in range(6):
        print "    ", "distance      :", i, distance.count(i)

    print "    ", "distance (avg):", float(sum(distance)) / float(len(distance))


    exact_match = 0
    acceptable = 0
    mismatch = 0

    for v_elem in verify["call_target"]:
        key = v_elem["origin"]
        process_key = False
        if key != "__libc_csu_init" and key != "__libc_csu_fini" and key != "_init" and key != "_start" and key != "_fini":
            process_key = key in gt_index.keys()
            if not process_key:
                print "ground_truth does not contain", key

        if process_key:
            gt_elem = gt_index[key]

            v_void = (v_elem["return_type"] == "0")
            gt_void = gt_elem["return_type"]["is_void"]

            if not gt_void and v_void:
                print "    ",
                print "return type mismatch (possibly problematic) verify vs ground truth for",
                print key
                print v_elem
                print gt_elem
                mismatch += 1
            else:
                if v_void == gt_void:
                    #print "    ", "exact return type match verify vs ground truth for", key
                    exact_match += 1
                else:
                    #print "    ", "return type mismatch(acceptable) verify vs ground truth for", key
                    acceptable += 1

    print "    ", "possibly problematic mismatches:", mismatch
    print "    ", "acceptable mismatches:", acceptable
    print "    ", "exact matches:", exact_match

def callsites_mergeable(gt_sites, v_sites):
    gt_site = gt_sites[0]
    for site in gt_sites:
        if site != gt_site:
            return False
    return True

def callsites_return_mergeable(gt_sites, v_sites):
    gt_site = gt_sites[0]
    for site in gt_sites:
        if site["return_type"] != gt_site["return_type"]:
            return False
    return True

def callsites_param_mergeable(gt_sites, v_sites):
    gt_site = gt_sites[0]
    for site in gt_sites:
        if site["params"] != gt_site["params"]:
            return False
    return True

def compare_call_site(ground_truth, verify):
    print "Comparing the set of call sites:"
    gt_index = {}
    v_index = {}
    for elem in ground_truth["call_site"]:
        __append_or_set_if_not_exist(gt_index, elem["origin"], [elem])

    for elem in verify["call_site"]:
        __append_or_set_if_not_exist(v_index, elem["origin"], [elem])

    print "GT", len(ground_truth["call_site"]), len(gt_index.keys())
    print "V ", len(verify["call_site"]), len(v_index.keys())

    print "GT - V", set(gt_index.keys()) - set(v_index.keys())
    print "V - GT", set(v_index.keys()) - set(gt_index.keys())

    not_acceptable = 0
    acceptable = 0
    exact_match = 0

    distance = []

    mismatches = []

    for key in v_index.keys():
        process_key = False
        if key != "__libc_csu_init" and key != "__libc_csu_fini" and key != "_init" and key != "_start" and key != "_fini":
            process_key = key in gt_index.keys()
            if not process_key:
                print "ground_truth does not contain", key

        if process_key:
            v_sites = v_index[key]
            gt_sites = gt_index[key]
            if len(v_sites) != len(gt_sites) and not callsites_param_mergeable(gt_sites, v_sites):
                mismatches += [(key, len(gt_sites), len(v_sites))]
            else:
                if callsites_param_mergeable(gt_sites, v_sites):
                    gt_sites = [gt_sites[0]] * len(v_sites)

                for index, (v_elem, gt_elem) in enumerate(zip(v_sites, gt_sites)):
                    v_count = int(v_elem["param_count"])
                    gt_count = int(gt_elem["param_count"])
                    if v_count < gt_count:
                        print "    ", "less params (not acceptable) in verify than in ground truth for", index, key
                        print v_elem
                        print gt_elem
                        print
                        not_acceptable += 1

                    if v_count == gt_count:
                        #print "    ", "exact param count match verify vs ground truth for", key
                        exact_match += 1

                    if v_count >= gt_count:
#                        if v_count - gt_count > 2:
#                            print "    ", "more params (acceptable) in verify than in ground truth for", key
#                            print v_elem
#                            print gt_elem
                            
                        acceptable += 1
                        distance += [v_count - gt_count]

    print "    ", "not acceptable functions:", not_acceptable
    print "    ", "acceptable functions:", acceptable
    print "    ", "exact matches:", exact_match

    for i in range(6):
        print "    ", "distance      :", i, distance.count(i)

    print "    ", "distance (avg):", float(sum(distance)) / float(len(distance))
    print

    for key, len_gt, len_v in mismatches:
        print "Callsite count missmatch", key, len_gt, "[GT]", len_v, "[V]"

    exact_match = 0
    acceptable = 0
    mismatch = 0
    mismatches = []

    for key in v_index.keys():
        process_key = False
        if key != "__libc_csu_init" and key != "__libc_csu_fini" and key != "_init" and key != "_start" and key != "_fini":
            process_key = key in gt_index.keys()
            if not process_key:
                print "ground_truth does not contain", key

        if process_key:
            v_sites = v_index[key]
            gt_sites = gt_index[key]
            if len(v_sites) != len(gt_sites) and not callsites_return_mergeable(gt_sites, v_sites):
                mismatches += [(key, len(gt_sites), len(v_sites))]
            else:
                if callsites_return_mergeable(gt_sites, v_sites):
                    gt_sites = [gt_sites[0]]

                for index, (v_elem, gt_elem) in enumerate(zip(v_sites, gt_sites)):
                    v_void = (v_elem["return_type"] == "0")
                    gt_void = gt_elem["return_type"]["is_void"]

                    if not v_void and gt_void:
                        print "    ",
                        print "return type mismatch (possibly problematic) verify vs ground truth for",
                        print key
                        print v_elem
                        print gt_elem
                        print
                        mismatch += 1
                    else:
                        if v_void == gt_void:
                            #print "    ", "exact return type match verify vs ground truth for", key
                            exact_match += 1
                        else:
                            #print "    ", "return type mismatch(acceptable) verify vs ground truth for", key
                            acceptable += 1

    print "    ", "possibly problematic mismatches:", mismatch
    print "    ", "acceptable mismatches:", acceptable
    print "    ", "exact matches:", exact_match

    for key, len_gt, len_v in mismatches:
        print "Callsite count missmatch", key, len_gt, "[GT]", len_v, "[V]"
        #for gt_site in gt_index[key]:
        #    print gt_site
        #for v_site in v_index[key]:
        #    print v_site

if len(sys.argv) < 3:
    sys.exit(-1)

fldr_path = sys.argv[1]
prg_name = sys.argv[2]

ground_truth = analyze_file_ground_truth(fldr_path, prg_name)
verify = analyze_file_verify(fldr_path, prg_name)

compare_address_taken(ground_truth, verify)
compare_call_target(ground_truth, verify)
compare_call_site(ground_truth, verify)
