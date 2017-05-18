
def __append_or_set_if_not_exist(dictionary, key, value):
    if key in dictionary.keys():
        dictionary[key] += value
    else:
        dictionary[key] = value
    return dictionary

def add_values_to_key(dictionary, key, values):
    dictionary = __append_or_set_if_not_exist(dictionary, key, [values])
    return dictionary

def get_values_for_key(dictionary, key):
    if key not in dictionary.keys():
        return []
    return dictionary[key]

def id(value):
    return value

def append_key_values(result, dictionary, key, fun=id):
    if key in dictionary:
        return result + key + ": " + str(fun(dictionary[key])) + "\n"
    else:
        return result + key + " not found" + "\n"

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

def copy_data(target, source, opt, key):
    if opt == "O2":
        add_values_to_key(target, key, float(source[key]))

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
