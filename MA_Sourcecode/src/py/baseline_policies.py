import math

import output_parsing as parsing
import utilities as utils
import verify_matching

import baseline_count
import baseline_type

def group_by_param_count(elems):
    buckets =  [[] for i in range(7)]

    for elem in elems:
        param_count = int(elem["param_count"])

        if param_count > 6:
            param_count = 6

        buckets[param_count] += [elem]

    return buckets

def get_acceptable_cs_names(cs_acceptable):
    cs_acceptable_names = []

    for (cs_name, len) in cs_acceptable:
        cs_acceptable_names += [cs_name]

    return cs_acceptable_names

def __weighted_expected_value(values):
    sites = sum([pair[0] for pair in values])

    if sites == 0:
        return 0

    targets = sum([(pair[0] * pair[1]) for pair in values])

    return float(targets) / float(sites)

def __weighted_value_median(values):
    sites = sum([pair[0] for pair in values])

    if sites == 0:
        return 0
    elif sites % 2 == 0:
        threshold = sites / 2
        val = 0
        index = 0
        result1 = 0
        while val < threshold:
            val += values[index][0]
            result1 = values[index][1]
            index += 1

        threshold = sites / 2 + 1
        val = 0
        index = 0
        result2 = 0
        while val < threshold:
            val += values[index][0]
            result2 = values[index][1]
            index += 1

        return (result1 + result2) / 2

    elif sites % 2 == 1:
        threshold = sites / 2 + 1
        val = 0
        index = 0
        result = 0
        while val < threshold:
            val += values[index][0]
            result = values[index][1]
            index += 1
        return result

def __weighted_value_std_deviation(values):
    sites = sum([pair[0] for pair in values])

    if sites == 0:
        return 0

    targets = sum([(pair[0] * pair[1] * pair[1]) for pair in values])

    expected = __weighted_expected_value(values)

    return math.sqrt((float(targets) / float(sites)) - expected * expected)

def generate_at_policy(clang, padyn, at_filter):
    (fn_acceptable, _, _) = verify_matching.call_target_matching(padyn, clang)
    at_acceptable = at_filter(padyn, clang, fn_acceptable)
    (cs_acceptable, _) = verify_matching.call_site_matching(padyn, clang, fn_acceptable)

    cs_acceptable_names = get_acceptable_cs_names(cs_acceptable)
    (css, _) = verify_matching.generate_index(clang["call_site"], cs_acceptable_names)
    (cts, _) = verify_matching.generate_index(clang["call_target"], at_acceptable)

    cs_buckets = group_by_param_count(css)
    ct_buckets = group_by_param_count(cts)

    at_policy = {}
    at_policy["policy"] = "AT"

    for (index, cs_bucket) in enumerate(cs_buckets):
        at_policy[str(index) + " cs"] = str(len(cs_bucket))

        if len(cs_bucket) > 0:
            at_policy[str(index) + " cts"] = str(len(at_acceptable)) + "$\\pm$ 0"
        else:
            at_policy[str(index) + " cts"] = "-"

    at_policy["summary cs"] = str(len(css))
    at_policy["summary cts"] = str(len(at_acceptable)) + "$\\pm$ 0"
    return at_policy

def restrict_to_two_decimals(value):
    return float(int(float(100) * value)) / float(100)

def generate_count_policy(clang, padyn, at_filter):
    schema_bucket_counts = baseline_count.analyse_pairing_baseline(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_count.group_by_cs_param_count(schema_bucket_counts)

    count_policy = {}
    count_policy["policy"] = "COUNT*"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        count_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            count_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            count_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

    count_policy["summary cs"] = str(sites)
    count_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)
    return count_policy

def generate_type_policy(clang, padyn, at_filter):
    schema_bucket_counts = baseline_type.analyse_pairing_baseline(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_type.group_by_cs_param_count(schema_bucket_counts)

    type_policy = {}
    type_policy["policy"] = "TYPE*"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        type_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            type_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            type_policy[str(index) + " cts"] = "-"


    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

    type_policy["summary cs"] = str(sites)
    type_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)
    return type_policy


def __all_functions_at(padyn, clang, fn_acceptable):
    return fn_acceptable

def __conserv_functions_at(padyn, clang, fn_acceptable):
    (at_acceptable, _, _) = verify_matching.address_taken_matching(clang, clang, fn_acceptable)
    return at_acceptable

def __ours_functions_at(padyn, clang, fn_acceptable):
    (at_acceptable, _, _) = verify_matching.address_taken_matching(padyn, padyn, fn_acceptable)
    return at_acceptable


def generate_safe(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    at_policy = generate_at_policy(clang, padyn, __all_functions_at)
    at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy(clang, padyn, __all_functions_at)
    count_policy["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy(clang, padyn, __all_functions_at)
    type_policy["target"] = prg_name

    return ([at_policy, count_policy, type_policy], "")

def generate_conserv(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    at_policy = generate_at_policy(clang, padyn, __conserv_functions_at)
    at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy(clang, padyn, __conserv_functions_at)
    count_policy["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy(clang, padyn, __conserv_functions_at)
    type_policy["target"] = prg_name

    return ([at_policy, count_policy, type_policy], "")

def generate_ours(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    at_policy = generate_at_policy(clang, padyn, __ours_functions_at)
    at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy(clang, padyn, __ours_functions_at)
    count_policy["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy(clang, padyn, __ours_functions_at)
    type_policy["target"] = prg_name

    return ([at_policy, count_policy, type_policy], "")


def generate_count_compare(clang, padyn):
    schema_bucket_counts = baseline_count.compare_pairing(clang, padyn)
    count_cs_ct_buckets = baseline_count.group_by_cs_param_count(schema_bucket_counts)

    count_policy = {}
    count_policy["policy"] = "COUNT"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        count_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            count_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            count_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

    count_policy["summary cs"] = str(sites)
    count_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)
    return count_policy

def generate_type_compare(clang, padyn):
    schema_bucket_counts = baseline_type.compare_pairing(clang, padyn)
    count_cs_ct_buckets = baseline_type.group_by_cs_param_count(schema_bucket_counts)

    type_policy = {}
    type_policy["policy"] = "TYPE"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        type_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            type_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            type_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

    type_policy["summary cs"] = str(sites)
    type_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)
    return type_policy

def compare_count(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    # at_policy = generate_at_policy(clang, padyn)
    # at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy(clang, padyn, __all_functions_at)
    count_policy["target"] = prg_name

    # generate Param Type policy baseline here
    # type_policy = generate_type_policy(clang, padyn)
    # type_policy["target"] = prg_name

    count_compare = generate_count_compare(clang, padyn)
    count_compare["target"] = prg_name


    return ([count_policy, count_compare], "")

def compare_type(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    # at_policy = generate_at_policy(clang, padyn)
    # at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    # count_policy = generate_count_policy(clang, padyn)
    # count_policy["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy(clang, padyn, __all_functions_at)
    type_policy["target"] = prg_name

    count_compare = generate_type_compare(clang, padyn)
    count_compare["target"] = prg_name


    return ([type_policy, count_compare], "")


def generate_at_policy__(clang, padyn, at_filter):
    (fn_acceptable, _, _) = verify_matching.call_target_matching(padyn, clang)
    at_acceptable = at_filter(padyn, clang, fn_acceptable)
    (cs_acceptable, _) = verify_matching.call_site_matching(padyn, clang, fn_acceptable)

    cs_acceptable_names = get_acceptable_cs_names(cs_acceptable)
    (css, _) = verify_matching.generate_index(clang["call_site"], cs_acceptable_names)
    (cts, _) = verify_matching.generate_index(clang["call_target"], at_acceptable)

    cs_buckets = group_by_param_count(css)
    ct_buckets = group_by_param_count(cts)

    at_policy = {}
    at_policy["policy"] = "AT"

    for (index, cs_bucket) in enumerate(cs_buckets):
        at_policy[str(index) + " cs"] = str(len(cs_bucket))

        if len(cs_bucket) > 0:
            at_policy[str(index) + " cts"] = str(len(at_acceptable)) + "$\\pm$ 0"
        else:
            at_policy[str(index) + " cts"] = "-"

    at_policy["summary cs"] = str(len(css))
    at_policy["summary cts avg"] = len(at_acceptable)
    at_policy["summary cts sig"] = 0
    at_policy["summary cts median"] = len(at_acceptable)
    return at_policy

def generate_count_policy__(clang, padyn, at_filter):
    schema_bucket_counts = baseline_count.analyse_pairing_baseline(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_count.group_by_cs_param_count(schema_bucket_counts)

    count_policy = {}
    count_policy["policy"] = "COUNT*"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        count_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            count_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            count_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    count_policy["summary cs"] = str(sites)
    count_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    count_policy["summary cts avg"] = str(avg)
    count_policy["summary cts sig"] = str(variance)
    count_policy["summary cts median"] = str(median)
    return count_policy

def generate_type_policy__(clang, padyn, at_filter):
    schema_bucket_counts = baseline_type.analyse_pairing_baseline(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_type.group_by_cs_param_count(schema_bucket_counts)

    type_policy = {}
    type_policy["policy"] = "TYPE*"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        type_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            type_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            type_policy[str(index) + " cts"] = "-"


    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    type_policy["summary cs"] = str(sites)
    type_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    type_policy["summary cts avg"] = str(avg)
    type_policy["summary cts sig"] = str(variance)
    type_policy["summary cts median"] = str(median)
    return type_policy



def generate_count_compare_safe(clang, fldr_path, prg_name, at_filter):
    padyn = parsing.parse_count_safe_verify(fldr_path, prg_name)
    schema_bucket_counts = baseline_count.compare_pairing(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_count.group_by_cs_param_count(schema_bucket_counts)

    count_policy = {}
    count_policy["policy"] = "COUNT_SAFE"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        count_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            count_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            count_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    count_policy["summary cs"] = str(sites)
    count_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    count_policy["summary cts avg"] = str(avg)
    count_policy["summary cts sig"] = str(variance)
    count_policy["summary cts median"] = str(median)
    return count_policy


def generate_count_compare_prec(clang, fldr_path, prg_name, at_filter):
    padyn = parsing.parse_count_prec_verify(fldr_path, prg_name)
    schema_bucket_counts = baseline_count.compare_pairing(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_count.group_by_cs_param_count(schema_bucket_counts)

    count_policy = {}
    count_policy["policy"] = "COUNT_PREC"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        count_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            count_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            count_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    count_policy["summary cs"] = str(sites)
    count_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    count_policy["summary cts avg"] = str(avg)
    count_policy["summary cts sig"] = str(variance)
    count_policy["summary cts median"] = str(median)
    return count_policy

def generate_type_compare_safe(clang, fldr_path, prg_name, at_filter):
    padyn = parsing.parse_type_safe_verify(fldr_path, prg_name)
    schema_bucket_counts = baseline_type.compare_pairing(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_type.group_by_cs_param_count(schema_bucket_counts)

    type_policy = {}
    type_policy["policy"] = "TYPE_SAFE"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        type_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            type_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            type_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    type_policy["summary cs"] = str(sites)
    type_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    type_policy["summary cts avg"] = str(avg)
    type_policy["summary cts sig"] = str(variance)
    type_policy["summary cts median"] = str(median)
    return type_policy


def generate_type_compare_prec(clang, fldr_path, prg_name, at_filter):
    padyn = parsing.parse_type_prec_verify(fldr_path, prg_name)
    schema_bucket_counts = baseline_type.compare_pairing(clang, padyn, at_filter)
    count_cs_ct_buckets = baseline_type.group_by_cs_param_count(schema_bucket_counts)

    type_policy = {}
    type_policy["policy"] = "TYPE_PREC"

    for (index, cs_ct_buckets) in enumerate(count_cs_ct_buckets):
        sites = sum([pair[0] for pair in cs_ct_buckets])
        avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
        variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))

        type_policy[str(index) + " cs"] = str(sites)
        if sites > 0:
            type_policy[str(index) + " cts"] = str(avg) + " $\\pm$ " + str(variance)
        else:
            type_policy[str(index) + " cts"] = "-"

    cs_ct_buckets = [item for sublist in count_cs_ct_buckets for item in sublist]

    sites = sum([pair[0] for pair in cs_ct_buckets])
    avg = restrict_to_two_decimals(__weighted_expected_value(cs_ct_buckets))
    variance = restrict_to_two_decimals(__weighted_value_std_deviation(cs_ct_buckets))
    median = restrict_to_two_decimals(__weighted_value_median(cs_ct_buckets))

    type_policy["summary cs"] = str(sites)
    type_policy["summary cts"] = str(avg) + " $\\pm$ " + str(variance)

    type_policy["summary cts avg"] = str(avg)
    type_policy["summary cts sig"] = str(variance)
    type_policy["summary cts median"] = str(median)
    return type_policy

def compare_all(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    at_policy = generate_at_policy__(clang, padyn, __all_functions_at)
    at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy__(clang, padyn, __all_functions_at)
    count_policy["target"] = prg_name

    count_compare_prec = generate_count_compare_prec(clang, fldr_path, prg_name, __all_functions_at)
    count_compare_prec["target"] = prg_name

    count_compare_safe = generate_count_compare_safe(clang, fldr_path, prg_name, __all_functions_at)
    count_compare_safe["target"] = prg_name

    type_compare_prec = generate_type_compare_prec(clang, fldr_path, prg_name, __all_functions_at)
    type_compare_prec["target"] = prg_name

    type_compare_safe = generate_type_compare_safe(clang, fldr_path, prg_name, __all_functions_at)
    type_compare_safe["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy__(clang, padyn, __all_functions_at)
    type_policy["target"] = prg_name

    return ([at_policy, type_policy, type_compare_prec, type_compare_safe, count_compare_prec, count_compare_safe, count_policy], "")

def compare_our_at_all(fldr_path, prg_name):
    padyn = parsing.parse_verify(fldr_path, prg_name)
    clang = parsing.parse_x86machine_ground_truth(fldr_path, prg_name)

    # generate Address Taken policy baseline here
    at_policy = generate_at_policy__(clang, padyn, __ours_functions_at)
    at_policy["target"] = prg_name

    # generate Param Count policy baseline here
    count_policy = generate_count_policy__(clang, padyn, __ours_functions_at)
    count_policy["target"] = prg_name

    count_compare_prec = generate_count_compare_prec(clang, fldr_path, prg_name, __ours_functions_at)
    count_compare_prec["target"] = prg_name

    count_compare_safe = generate_count_compare_safe(clang, fldr_path, prg_name, __ours_functions_at)
    count_compare_safe["target"] = prg_name

    type_compare_prec = generate_type_compare_prec(clang, fldr_path, prg_name, __ours_functions_at)
    type_compare_prec["target"] = prg_name

    type_compare_safe = generate_type_compare_safe(clang, fldr_path, prg_name, __ours_functions_at)
    type_compare_safe["target"] = prg_name

    # generate Param Type policy baseline here
    type_policy = generate_type_policy__(clang, padyn, __ours_functions_at)
    type_policy["target"] = prg_name

    return ([at_policy, type_policy, type_compare_prec, type_compare_safe, count_compare_prec, count_compare_safe, count_policy], "")

