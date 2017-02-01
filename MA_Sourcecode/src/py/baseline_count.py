import baseline_utils

def __get_bucket_id(data):
    count = int(data["param_count"])

    bucket_id = str(count) + " "
    bucket_id += "v"
    #if bool(data["return_type"]["is_void"]):
    #    bucket_id += "v"
    #else:
    #    bucket_id += "n"

    return bucket_id

def __bucket_id_count(bucket_id):
    return int(bucket_id[:-1])

def __bucket_id_void(bucket_id):
    return bucket_id[-1] == "v"

def __is_schema_id_match(cs_id, ct_id):
    if cs_id == ct_id:
        return True

    if __bucket_id_void(cs_id) or (__bucket_id_void(cs_id) == __bucket_id_void(ct_id)):
        if __bucket_id_count(cs_id) >= __bucket_id_count(ct_id):
            return True

    return False

def analyse_pairing_baseline(clang, padyn, at_filter):
    return baseline_utils.analyse_pairing_baseline(clang, padyn, __get_bucket_id, __is_schema_id_match, at_filter)


def __identity_function(padyn, clang, fn_acceptable):
    return fn_acceptable


def compare_pairing(clang, padyn, at_filter = __identity_function):
    return baseline_utils.analyse_pairing_padyn(clang, padyn, __get_bucket_id, __is_schema_id_match, at_filter)

def group_by_cs_param_count(schema_bucket_counts):
    count_cs_buckets = [[] for i in range(7)]

    for (cs_bucket_id, cs_count, ct_count) in schema_bucket_counts:
        param_count = __bucket_id_count(cs_bucket_id)
        if param_count > 6:
            param_count = 6

        count_cs_buckets[param_count] += [(cs_count, ct_count)]

    return count_cs_buckets

def generate_cdf_data(clang, padyn):
    return baseline_utils.generate_cdf_data(clang, padyn, __get_bucket_id, __is_schema_id_match)

def generate_cdf_data_real(clang, padyn):
    return baseline_utils.generate_cdf_data_real(clang, padyn, __get_bucket_id, __is_schema_id_match)
