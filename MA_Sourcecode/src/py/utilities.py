import os

def __append_or_set_if_not_exist(dictionary, key, value):
    if key in dictionary.keys():
        dictionary[key] += value
    else:
        dictionary[key] = value
    return dictionary

def add_values_to_key(result, key, values):
    result = __append_or_set_if_not_exist(result, key, [values])
    return result

def is_internal_function(fnname):
    internal_functions = []
    internal_functions += ["deregister_tm_clones"]
    internal_functions += ["register_tm_clones"]
    internal_functions += ["__do_global_dtors_aux"]
    internal_functions += ["frame_dummy"]
    internal_functions += ["__libc_csu_init"]
    internal_functions += ["__libc_csu_fini"]
    internal_functions += ["_init"]
    internal_functions += ["_start"]
    internal_functions += ["_fini"]

    return fnname in internal_functions
