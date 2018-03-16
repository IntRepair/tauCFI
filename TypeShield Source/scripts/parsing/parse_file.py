import os

def _decode_type_clang(type_string):
    type_string = type_string.strip()

    relevant = True
    type_info = {}
    type_info["is_void"] = False
    type_info["is_pointer"] = False
    type_info["is_float"] = False
    type_info["is_problem"] = False
    if type_string == "":
        relevant = False
    if type_string == "void":
        type_info["wideness"] = 0
        type_info["is_void"] = True
    elif type_string.endswith("*"):
        type_info["wideness"] = 64 #TODO find a better way to do this...
        type_info["is_pointer"] = True
    elif type_string in ["u64", "i64"]:
        type_info["wideness"] = 64
    elif type_string in ["u32", "i48"]:
        type_info["wideness"] = 48
    elif type_string in ["u32", "i32"]:
        type_info["wideness"] = 32
    elif type_string in ["u16", "i16"]:
        type_info["wideness"] = 16
    elif type_string in ["u8", "i8"]:
        type_info["wideness"] = 8
    elif type_string in ["u1", "i1"]:
        type_info["wideness"] = 1
    elif type_string in ["float"]:
        type_info["wideness"] = 32
        type_info["is_float"] = True
        relevant = False
    elif type_string in ["double"]:
        type_info["wideness"] = 64
        type_info["is_float"] = True
        relevant = False
    else:
        type_info["is_problem"] = True
        type_info["raw"] = type_string

    return (relevant, type_info)

def _decode_type_verify(type_string):
    type_string = type_string.strip()

    relevant = True
    type_info = {}
    type_info["is_void"] = False
    type_info["is_pointer"] = False
    type_info["is_float"] = False
    type_info["is_problem"] = False
    if type_string == "":
        relevant = False
    if type_string == "0":
        type_info["wideness"] = 0
        type_info["is_void"] = True
    elif type_string in ["4", "x"]:
        type_info["wideness"] = 64
    elif type_string == "3":
        type_info["wideness"] = 32
    elif type_string == "2":
        type_info["wideness"] = 16
    elif type_string == "1":
        type_info["wideness"] = 8
    else:
        type_info["is_problem"] = True
        type_info["raw"] = type_string

    return (relevant, type_info)

def _sanitize_symbol(symbol):
    if symbol.endswith("\\000"):
        symbol = symbol[:-4]
    return symbol

# TODO: add to decode_type_fn
#        elif not param_type["is_float"]:
#            print "unusable_type", param_token, param_type

#   0       1           2               3                   4       5           6           7
# <MFNX>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;
# <AMCS>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;
# <MFN>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;

def _map_line_to_values(line, index, decode_type_fn):
    values = {}
    params = []

    line = line.strip()
    tokens = line.split(";")

    values["index"] = index
    values["mang_name"] = _sanitize_symbol(tokens[1])
    values["demang_name"] = _sanitize_symbol(tokens[2])
    values["module"] = tokens[3]

    flag_tokens = tokens[4].split("|")
    flags = []    
    for flag_token in flag_tokens:
        flag_token = flag_token.strip()
        if flag_token != "":
            flags += [flag_token]
    values["flags"] = flags

    param_tokens = tokens[5].split("|")
    params = []
    problems = []
    param_index = 0
    param_count = 0
    for param_token in param_tokens:
        (relevant, param_type) = decode_type_fn(param_token)
        if relevant:
            if not param_type["is_problem"]:
                params += [param_type]
                # We truncate trailing void parameters
                if not param_type["is_void"]:
                    param_count = param_index + 1
            else:
                problems += [(param_index, param_type)]
            param_index += 1

    if [] != problems:
        values["problems"] = problems

    values["params"] = params
    values["param_count"] = param_count
    #TODO get the correct return_type for the callsite from clang
    #   Our tool works in that regard
    #values["ret_type"] = decode_type_fn(tokens[7])
    values["ret_type"] = _decode_type_clang("void")

    return values;

def map_line_to_values_verify(line, index):
    return _map_line_to_values(line, index, _decode_type_verify)

def map_line_to_values_clang(line, index):
    entry = _map_line_to_values(line, index, _decode_type_clang)
    return entry

def map_line_to_values_module(line):
    entry = {}

    tokens = line.split(";")
    module_name = tokens[0].strip()
    module_path = tokens[2].strip()

    entry["module"] = os.path.join(module_path, module_name)
    return entry

def map_line_to_values_linker(line):
    tokens = line.split(" ")
    cur_path = tokens[1].strip()
    binary_name = tokens[2].strip()
    object_file = tokens[3].strip()
    source_file = tokens[4].strip()

    entry = {}

    if object_file.startswith("/"):
        # -> print warning, as file is a system file
        # -> ignore, as we have currently no solution to get the actual source file in clang
        return entry
    elif source_file == "":
        # no source file found -> print error
        return entry
    else:
        entry["binary"] = os.path.basename(binary_name)

        path = os.path.join(cur_path, source_file)
        entry["source"] = path
        # -> append cur_path to object_file, replace object_file_name with source_file name
        path = os.path.join(cur_path, object_file)
        path = os.path.dirname(path)
        path = os.path.join(path, source_file)
        entry["source2"] = path

        return entry
