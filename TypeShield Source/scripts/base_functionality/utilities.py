import math

def add_values_to_key(dictionary, key, values):
    if key not in dictionary.keys():
        dictionary[key] = []
    dictionary[key] += [values]

def get_values_for_key(dictionary, key):
    if key not in dictionary.keys():
        dictionary[key] = []
    return dictionary[key]

def pop_values_for_key(dictionary, key):
    return dictionary.pop(key, [])

def add_values_to_subkey(dictionary, key, subkey, value):
    if key not in dictionary.keys():
        dictionary[key] = {}
    add_values_to_key(dictionary[key], subkey, value)

def get_values_for_subkey(dictionary, key, subkey):
    if key not in dictionary.keys():
        dictionary[key] = {}
    return get_values_for_key(dictionary[key], subkey)

def pop_values_for_subkey(dictionary, key, subkey):
    if key not in dictionary.keys():
        dictionary[key] = {}
    return pop_values_for_key(dictionary[key], subkey)

def is_internal_function(fnname):
    internal_functions = []
    internal_functions += ["deregister_tm_clones"]
    internal_functions += ["register_tm_clones"]
    internal_functions += ["__do_global_dtors_aux"]
    internal_functions += ["frame_dummy"]
    internal_functions += ["__libc_csu_init"]
    internal_functions += ["__libc_start_main"]
    internal_functions += ["__libc_csu_fini"]
    internal_functions += ["_init"]
    internal_functions += ["_start"]
    internal_functions += ["_fini"]

    return fnname in internal_functions

def geomean(values):
    if len(values) < 1:
        return ""

    result = 1.0

    offset = False
    for value in values:
        if float(value) == 0.0:
            offset = True

    if not offset:
        for value in values:
            result *= float(value)
        return str(float(int(pow(result, 1.0 / float(len(values))) * 100)) / float(100))
    else:
        for value in values:
            result *= float(value + 1.0)
        return str(float(int((pow(result, 1.0 / float(len(values))) - 1.0) * 100)) / float(100))

def gen_pct_string(value):
    return str(float(int(float(value) * float(10000))) / float(100))

def equal_dicts(d1, d2, ignore_keys):
    ignored = set(ignore_keys)
    for k1, v1 in d1.iteritems():
        if k1 not in ignored and (k1 not in d2 or d2[k1] != v1):
            return False
    for k2, v2 in d2.iteritems():
        if k2 not in ignored and k2 not in d1:
            return False
    return True

def expected_value(values):
    if len(values) == 0:
        return 0

    return float(sum(values)) / float(len(values))

def weighted_expected_value(values):
    sites = sum([pair[0] for pair in values])

    if sites == 0:
        return 0

    targets = sum([(pair[0] * pair[1]) for pair in values])
    return float(targets) / float(sites)

def std_deviation(values):
    if len(values) == 0:
        return 0

    targets = sum([value * value for value in values])
    expected = expected_value(values)

    return math.sqrt((float(targets) / float(len(values))) - expected * expected)

def weighted_std_deviation(values):
    sites = sum([pair[0] for pair in values])

    if sites == 0:
        return 0

    targets = sum([(pair[0] * pair[1] * pair[1]) for pair in values])

    expected = weighted_expected_value(values)

    return math.sqrt((float(targets) / float(sites)) - expected * expected)

def median(values):
    if len(values) == 0:
        return 0
    elif len(values) % 2 == 0:

        result1 = 0
        for index in range(len(values) / 2):
            result1 = values[index]

        result2 = 0
        for index in range(len(values) / 2 + 1):
            result2 = values[index]

        return (result1 + result2) / 2

    elif len(values) % 2 == 1:
        result2 = 0
        for index in range(len(values) / 2 + 1):
            result2 = values[index]
        return result2

def weighted_value_median(values):
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
