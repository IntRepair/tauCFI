import os
import base_functionality.utilities as utils
import parse_file as fparser

def entry_identity(entry):
    return (entry["demang_name"], entry["mang_name"])

def _analyze_file_line_verify(result, line, index):
    if line.count("<FN>") > 0:
        entry = fparser.map_line_to_values_verify(line, index)

	entry_id = entry_identity(entry)

        if not utils.is_internal_function(entry_id[0]):
            if entry_id not in utils.get_values_for_key(result, "ct_index"):
                utils.add_values_to_key(result, "call_target", entry)
                utils.add_values_to_key(result, "ct_index", entry_id)
            else:
                utils.add_values_to_subkey(result, "double_vals", "call_target", entry)
                utils.add_values_to_subkey(result, "double_vals", "ct_index", entry_id)

            if "AT" in entry["flags"]:
                if entry_id not in utils.get_values_for_key(result, "at_index"):
                    utils.add_values_to_key(result, "address_taken", entry)
                    utils.add_values_to_key(result, "at_index", entry_id)
                else:
                    utils.add_values_to_subkey(result, "double_vals", "adress_taken", entry)
                    utils.add_values_to_subkey(result, "double_vals", "at_index", entry_id)

            if "PLT" not in entry["flags"]:
                if entry_id not in utils.get_values_for_key(result, "cfn_index"):
                    utils.add_values_to_key(result, "compiled_function", entry)
                    utils.add_values_to_key(result, "cfn_index", entry_id)
                else:
                    utils.add_values_to_subkey(result, "double_vals", "compiled_functions", entry)
                    utils.add_values_to_subkey(result, "double_vals", "cfn_index", entry_id)

    elif line.count("<CS>") > 0:
        entry = fparser.map_line_to_values_verify(line, index)

        entry_id = entry_identity(entry)

        if not utils.is_internal_function(entry_id[0]):
            utils.add_values_to_key(result, "call_site", entry)
            utils.add_values_to_subkey(result, "cs_index", entry_id, entry)

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

    _analyze_verify_file_lines(file_path, result)

    file_path = os.path.join(path, "verify." + tag_cs + "." + name + ".cs")
    if not os.path.isfile(file_path):
        file_path = os.path.join(path, "verify." + name + ".cs")

    _analyze_verify_file_lines(file_path, result)

    return result
