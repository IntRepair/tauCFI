import utilities as utils
import verify_matching

def is_void_type(type):
    if isinstance(type, dict):
        return type["is_void"]

    return type == "void"

def __generate_buckets(clang, padyn, get_bucket_id):
    (fn_acceptable, _, _) = verify_matching.call_target_matching(padyn, clang)
#    (at_acceptable, _, _) = verify_matching.address_taken_matching(padyn, clang, fn_acceptable)
    (cs_acceptable, _) = verify_matching.call_site_matching(padyn, clang, fn_acceptable)

    css = [pair[0] for pair in cs_acceptable] 

    cs_buckets = {}
    for callsite in clang["call_site"]:
        if (callsite["origin"]) in css:
            utils.add_values_to_key(cs_buckets, get_bucket_id(callsite), callsite)

    ct_buckets = {}
    for calltarget in clang["call_target"]:
        if calltarget["origin"] in fn_acceptable:
            utils.add_values_to_key(ct_buckets, get_bucket_id(calltarget), calltarget)

    return (cs_buckets, ct_buckets)

def analyse_pairing_baseline(clang, padyn, get_bucket_id, is_exact_id_match, is_schema_id_match):
    (cs_buckets, ct_buckets) = __generate_buckets(clang, padyn, get_bucket_id)

#    exact_targets = {}
    schema_targets = {}
    for cs_bucket_id in cs_buckets.keys():
        for ct_bucket_id in ct_buckets.keys():
#            if is_exact_id_match(cs_bucket_id, ct_bucket_id):
#                utils.add_values_to_key(exact_targets, cs_bucket_id, ct_bucket_id)
            if is_schema_id_match(cs_bucket_id, ct_bucket_id):
                utils.add_values_to_key(schema_targets, cs_bucket_id, ct_bucket_id)

#    exact_bucket_counts = []
#    result += "[EXACT]\n"
#    result += "bucket_count:" + str(len(exact_targets.keys())) + "\n"
#    for cs_bucket_id in exact_targets.keys():
#        target_count = 0
#        for target_key in exact_targets[cs_bucket_id]:
#            target_count += len(ct_buckets[target_key])
#        exact_bucket_counts += [target_count]
#        result += "(" + cs_bucket_id + "," + str(target_count) + ")\n"
#
#    result += "bucket_element_average:" + str(__expected_value(exact_bucket_counts))
#    result += "bucket_element_variance:" + str(__value_variance(exact_bucket_counts))

    schema_bucket_counts = []
#    result += "[SCHEMA]\n"
#    result += "bucket_count:" + str(len(schema_targets.keys())) + "\n"
    for cs_bucket_id in schema_targets.keys():
        target_count = 0
        for target_key in schema_targets[cs_bucket_id]:
            target_count += len(ct_buckets[target_key])
        schema_bucket_counts += [(cs_bucket_id, len(cs_buckets[cs_bucket_id]), target_count)]
#        result += "(" + cs_bucket_id + "," + str(target_count) + ")\n"

#    result += "bucket_element_average:" + str(__expected_value(schema_bucket_counts))
 #   result += "bucket_element_variance:" + str(__value_variance(schema_bucket_counts))

    return schema_bucket_counts

def generate_padyn_to_clang_cs_index(clang, padyn):
    (fn_acceptable, _, _) = verify_matching.call_target_matching(padyn, clang)
#    (at_acceptable, _, _) = verify_matching.address_taken_matching(padyn, clang, fn_acceptable)
    (cs_acceptable, _) = verify_matching.call_site_matching(padyn, clang, fn_acceptable)

    css = [pair[0] for pair in cs_acceptable]

    (_, padyn_csI) = verify_matching.generate_index(padyn["call_site"], css)
    (_, clang_csI) = verify_matching.generate_index(clang["call_site"], css)

    padyn_to_clang_cs_index = {}

    for cs_name in css:
        for (padyn_cs, clang_cs) in zip(padyn_csI[cs_name], clang_csI[cs_name]):
            padyn_to_clang_cs_index[padyn_cs["index"]] = clang_cs["index"]

    return padyn_to_clang_cs_index

def generate_clang_index_to_clang_bucket_id(clang, padyn, get_bucket_id):

    (cs_buckets, _) = __generate_buckets(clang, padyn, get_bucket_id)

    clang_index_to_clang_bucket_id = {}
    for cs_bucket_id in cs_buckets.keys():
        for callsite in cs_buckets[cs_bucket_id]:
            clang_index_to_clang_bucket_id[callsite["index"]] = cs_bucket_id

    return clang_index_to_clang_bucket_id

def generate_padyn_cs_index_to_clang_bucket_id(clang, padyn, get_bucket_id):
    padyn_to_clang_index = generate_padyn_to_clang_cs_index(clang, padyn)

    clang_index_to_clang_bucket_id = generate_clang_index_to_clang_bucket_id(clang, padyn, get_bucket_id)

    return (padyn_to_clang_index, clang_index_to_clang_bucket_id)


def analyse_pairing_padyn(clang, padyn, get_bucket_id, is_exact_id_match, is_schema_id_match):
    (cs_buckets, ct_buckets) = __generate_buckets(padyn, clang, get_bucket_id)

    schema_targets = {}
    for cs_bucket_id in cs_buckets.keys():
        for ct_bucket_id in ct_buckets.keys():
            if is_schema_id_match(cs_bucket_id, ct_bucket_id):
                utils.add_values_to_key(schema_targets, cs_bucket_id, ct_bucket_id)

    schema_bucket_counts_padyn = []
    for cs_bucket_id in schema_targets.keys():
        target_count = 0
        for target_key in schema_targets[cs_bucket_id]:
            target_count += len(ct_buckets[target_key])
        schema_bucket_counts_padyn += [(cs_bucket_id, len(cs_buckets[cs_bucket_id]), target_count)]

    (P2CI, CI2CBI) = generate_padyn_cs_index_to_clang_bucket_id(clang, padyn, get_bucket_id)

    schema_targets_clang = {}
    for (cs_bucket_id, _, target_count) in schema_bucket_counts_padyn:
        for callsite in cs_buckets[cs_bucket_id]:
            key = CI2CBI[P2CI[callsite["index"]]]
            utils.add_values_to_key(schema_targets_clang, key, target_count)

    schema_bucket_counts = []
    for cs_bucket_id in schema_targets_clang.keys():
        for target_count in schema_targets_clang[cs_bucket_id]:
            schema_bucket_counts += [(cs_bucket_id, 1, target_count)]

    return schema_bucket_counts


def generate_cdf_data(clang, padyn, get_bucket_id, is_exact_id_match, is_schema_id_match):
    (cs_buckets, ct_buckets) = __generate_buckets(clang, padyn, get_bucket_id)

    cdf_bins = []
    for cs_bucket_id in cs_buckets.keys():
        targets = []
        for ct_bucket_id in ct_buckets.keys():
            if is_schema_id_match(cs_bucket_id, ct_bucket_id):
                targets += [(ct_bucket_id, i) for i in range(len(ct_buckets[ct_bucket_id]))]

        cdf_bins += [(cs_bucket_id, set(targets))]

    cdf_values = []

    for _ in range(len(cdf_bins)):
        cdf_bins.sort(key=lambda x: len(x[1]), reverse=True)
        (cs_bucket_id, targets) = cdf_bins.pop()
        print (cs_bucket_id, len(targets))
        site_count = len(cs_buckets[cs_bucket_id])
        cdf_values += [(cs_bucket_id, site_count, len(targets))]
        cdf_bins = map(lambda (bucket_id_, targets_): (bucket_id_, targets_.union(targets)), cdf_bins)

    return cdf_values
