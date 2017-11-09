import os
import base_functionality.utilities as utils
import parse_file as fparser

def _analyze_file_line_verify(result, line, index):
    if line.count("<FN>") > 0:
        entry = fparser.map_line_to_values_verify(line, index)

        name_pair = (entry["demang_name"], entry["mang_name"])

        if not utils.is_internal_function(name_pair[0]):
            if name_pair not in utils.get_values_for_key(result, "ct_index"):
                utils.add_values_to_key(result, "call_target", entry)
                utils.add_values_to_key(result, "ct_index", name_pair)
            else:
                utils.add_values_to_subkey(result, "double_vals", "call_target", entry)
                utils.add_values_to_subkey(result, "double_vals", "ct_index", name_pair)

            if "AT" in entry["flags"]:
                if name_pair not in utils.get_values_for_key(result, "at_index"):
                    utils.add_values_to_key(result, "address_taken", entry)
                    utils.add_values_to_key(result, "at_index", name_pair)
                else:
                    utils.add_values_to_subkey(result, "double_vals", "adress_taken", entry)
                    utils.add_values_to_subkey(result, "double_vals", "at_index", name_pair)

            if "PLT" not in entry["flags"]:
                if name_pair not in utils.get_values_for_key(result, "cfn_index"):
                    utils.add_values_to_key(result, "compiled_function", entry)
                    utils.add_values_to_key(result, "cfn_index", name_pair)
                else:
                    utils.add_values_to_subkey(result, "double_vals", "compiled_functions", entry)
                    utils.add_values_to_subkey(result, "double_vals", "cfn_index", name_pair)

    elif line.count("<CS>") > 0:
        entry = fparser.map_line_to_values_verify(line, index)

        if not utils.is_internal_function(entry["demang_name"]):
            utils.add_values_to_key(result, "call_site", entry)

def _analyze_verify_file_lines(file_path, result):
    with file(file_path) as in_file:
        index = 0
        for line in in_file:
            _analyze_file_line_verify(result, line, index)
            index += 1

def parse_verify_basic(path, name):
    result = {}
    _analyze_verify_file_lines(os.path.join(path, "verify." + name + ".ct"), result)
    _analyze_verify_file_lines(os.path.join(path, "verify." + name + ".cs"), result)
    return result

def parse_verify(path, name, tag_ct, tag_cs):
    result = {}

    file_path = os.path.join(path, "verify." + tag_ct + "." + name + ".ct")
    if not os.path.isfile(file_path):
        file_path = os.path.join(path, "verify." + name + ".ct")

    result = _analyze_verify_file_lines(file_path, result)

    file_path = os.path.join(path, "verify." + tag_cs + "." + name + ".cs")
    if not os.path.isfile(file_path):
        file_path = os.path.join(path, "verify." + name + ".cs")

    result = _analyze_verify_file_lines(file_path, result)

    return result
