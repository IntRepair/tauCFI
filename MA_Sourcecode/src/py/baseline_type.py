import baseline_utils

def __is_acceptable_pair(callsite, calltarget):
    if int(calltarget["param_count"]) > int(callsite["param_count"]):
        return False

    if not is_void(callsite["return_type"]) and is_void(calltarget["return_type"]):
        return False

    return True

def __is_exact_pair(callsite, calltarget):
    if int(calltarget["param_count"]) != int(callsite["param_count"]):
        return False

    ct_params = calltarget["params"]
    cs_params = callsite["params"]

    for ct_param, cs_param in zip(ct_params, cs_params):
        if (not bool(ct_param["is_empty"])) and (not bool(cs_param["is_empty"])):
            if (not "wideness" in ct_param.keys()) or (not "wideness" in cs_param.keys()):
                print ct_param
                print cs_param
            if int(ct_param["wideness"]) != int(cs_param["wideness"]):
                return False
        elif bool(ct_param["is_empty"]) != bool(cs_param["is_empty"]):
            return False

    # skip void callsites, as they can call all targets
    if is_void(callsite["return_type"]):
        return True

    if calltarget["return_type"]["wideness"] != callsite["return_type"]["wideness"]:
        return False

    return True

def __get_bucket_id(data):
    bucket_id = str(data["param_count"]) + " "

    for param in data["params"]:
        bucket_id += str(param["wideness"]).strip() + " "

    bucket_id += str(data["return_type"]["wideness"])

    return bucket_id

def __bucket_id_count(bucket_id):
    return int(bucket_id.split(" ")[0])

def __bucket_id_return_type(bucket_id):
    return int(bucket_id.split(" ")[-1])

def __bucket_id_param_types(bucket_id):
    return map(int, bucket_id.split(" ")[1:-1])

def __is_exact_id_match(cs_id, ct_id):
    if cs_id == ct_id:
        return True

    if __bucket_id_count(cs_id) == __bucket_id_count(ct_id):
        if __bucket_id_return_type(cs_id) == 0 or __bucket_id_return_type(cs_id) == __bucket_id_return_type(ct_id):
            for (ct_type, cs_type) in zip(__bucket_id_param_types(cs_id), __bucket_id_param_types(ct_id)):
                if ct_type == cs_type:
                    return True

    return False

def __is_schema_id_match(cs_id, ct_id):
    if cs_id == ct_id:
        return True

    if __bucket_id_count(cs_id) >= __bucket_id_count(ct_id):
        if __bucket_id_return_type(cs_id) <= __bucket_id_return_type(ct_id):
            for (ct_type, cs_type) in zip(__bucket_id_param_types(cs_id), __bucket_id_param_types(ct_id)):
                if ct_type <= cs_type:
                    return True
    return False

def analyse_pairing_baseline(clang, padyn):
    return baseline_utils.analyse_pairing_baseline(clang, padyn, __get_bucket_id, __is_exact_id_match, __is_schema_id_match)

def compare_pairing(clang, padyn):
    return baseline_utils.analyse_pairing_padyn(clang, padyn, __get_bucket_id, __is_exact_id_match, __is_schema_id_match)

def group_by_cs_param_count(schema_bucket_counts):
    count_cs_buckets = [[] for i in range(7)]

    for (cs_bucket_id, cs_count, ct_count) in schema_bucket_counts:
        param_count = __bucket_id_count(cs_bucket_id)
        if param_count > 6:
            param_count = 6

        count_cs_buckets[param_count] += [(cs_count, ct_count)]

    return count_cs_buckets

def generate_cdf_data(clang, padyn):
    return baseline_utils.generate_cdf_data(clang, padyn, __get_bucket_id, __is_exact_id_match, __is_schema_id_match)
