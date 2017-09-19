
def add_values_to_key(dictionary, key, values):
    if key not in dictionary.keys():
        dictionary[key] = []
    dictionary[key] += [values]

def get_values_for_key(dictionary, key):
    if key not in dictionary.keys():
        dictionary[key] = []
    return dictionary[key]

def add_values_to_subkey(dictionary, key, subkey, value):
    if key not in dictionary.keys():
        dictionary[key] = {}
    add_values_to_key(dictionary[key], subkey, value)

def get_values_for_subkey(dictionary, key, subkey):
    if key not in dictionary.keys():
        dictionary[key] = {}
    return get_values_for_key(dictionary[key], subkey)

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
