import os
import utilities as utils

#ngx_atoi;               <(M)FN>;   i8*;                    i64; ; ; ; ; parameter_count 2;return_type i64;
#ngx_sort;               <(M)CS>;   i8*;                    i8*; ; ; ; ; parameter_count 2;return_type i64;
#ngx_clean_old_cycles;   <(M)AT>;   %struct.ngx_event_s*;   ;    ; ; ; ; parameter_count 1;return_type void;

def is_typename(symbol):
    return symbol in ["double", "float", "void", "u8", "i8", "u16", "i16", "u32", "i32", "u64", "i64"]

def __sanitize_symbol(symbol):

    # here we remove the parameters
    tokens = symbol.split("(")
    if len(tokens) > 1:
        to_be_deleted = tokens[-1]
        tokens = tokens[:-1]
        right_par_count = to_be_deleted.count(")") - 1

        while right_par_count > 0:
            to_be_deleted = tokens[-1]
            tokens = tokens[:-1]
            right_par_count += to_be_deleted.count(")") - 1

        symbol = "(".join(tokens)

    #here we remove the
    prefix = []
    tokens = symbol.split(" ")
    if len(tokens) > 3 and tokens[0] == "non-virtual" and tokens[1] == "thunk" and tokens[2] == "to":
        prefix = tokens[1:3]
        tokens = tokens[3:]

    if is_typename(tokens[0]):
        tokens = tokens[1:]
    elif tokens[0].count("<") > 0 and len(tokens) > 1:
        token = tokens[0]
        remainder_token = tokens[1:]
        par_token = [token]
        left_param_count = token.count("(")
        right_param_count = token.count(")")
        left_temp_count = token.count("<")
        right_temp_count = token.count(">")
        while left_temp_count > right_temp_count or left_param_count > right_param_count:
            token = remainder_token[0]
            remainder_token = remainder_token[1:]
            par_token += [token]
            left_param_count += token.count("(")
            right_param_count += token.count(")")
            left_temp_count += token.count("<")
            right_temp_count += token.count(">")

        if len(remainder_token) > 0:
            tokens = remainder_token
            #print " ".join(par_token), " <--> ", " ".join(remainder_token)
    elif len(tokens) > 1:
        tokens = tokens[1:]

    if "v8::internal::compiler::" == " ".join(prefix + tokens):
        print
        print symbol
        print

    return " ".join(prefix + tokens)

def __decode_type(type_string):
    type_string = type_string.strip()

    type_info = {}
    type_info["raw"] = type_string
    type_info["is_empty"] = (type_string == "")
    type_info["is_void"] = (type_string == "void")
    type_info["usable"] = False
    if "*" in type_string:
        type_info["is_pointer"] = True
        type_info["usable"] = True
        type_info["wideness"] = 64
    elif not type_info["is_empty"]:
        type_info["is_pointer"] = False
        type_info["usable"] = True
        if type_string == "u64" or type_string == "i64":
            type_info["wideness"] = 64
        elif type_string == "u48" or type_string == "i48":
            type_info["wideness"] = 48
        elif type_string == "u32" or type_string == "i32":
            type_info["wideness"] = 32
        elif type_string == "u16" or type_string == "i16":
            type_info["wideness"] = 16
        elif type_string == "u8" or type_string == "i8":
            type_info["wideness"] = 8
        elif type_string == "u1" or type_string == "i1":
            type_info["wideness"] = 1
        elif type_info["is_void"]:
            type_info["wideness"] = 0
        else:
            type_info["usable"] = False

    return type_info

def __decode_padyn_type(type_string):
    type_string = type_string.strip()

    type_info = {}
    type_info["raw"] = type_string
    type_info["is_empty"] = (type_string == "")
    type_info["is_void"] = (type_string == "0")
    type_info["usable"] = False
    if not type_info["is_empty"]:
        type_info["usable"] = True

        if type_string == "4" or type_string == "x":
            type_info["wideness"] = 64
        elif type_string == "3":
            type_info["wideness"] = 32
        elif type_string == "2":
            type_info["wideness"] = 16
        elif type_string == "1":
            type_info["wideness"] = 8
        elif type_info["is_void"]:
            type_info["wideness"] = 0
        else:
            type_info["usable"] = False

    return type_info

def __map_line_to_values(line, index):
    values = {}
    params = []


    line = line.strip()
    tokens = line.split(";")

    values["index"] = index
    values["param_count"] = tokens[-3].strip().split(" ")[1]
    return_type = __decode_type(tokens[-2].split(" ")[1])
    if return_type["raw"] == "{": # for now only look at the first, find out what happens when there are two return type values (maybe pair ?)
        return_type = __decode_type(tokens[-2].split(" ")[2][:-1]) # [:-1] to remove the trailing comma

    if return_type["usable"]:
        values["return_type"] = return_type
    elif not return_type["raw"] in ["float", "double"]:
        print line
        print "unusable_return_type", return_type
        values["return_type"] = __decode_type("void")
    else:
        # for now we assume void when involing floating point stuff
        values["return_type"] = __decode_type("void")

    for index, token in enumerate(tokens):
        token = token.strip()
        if index == 0:
            values["origin"] = __sanitize_symbol(token)
            #values["raw_origin"] = token
#        elif index == 1:
#            values["tag"] = token
        elif index == 2:
            values["module"] = token.split(" ")[1]
        elif index > 2 and (index - 3) < int(values["param_count"]):
            param_type = __decode_type(token)
            if param_type["usable"]:
                params += [param_type]
            elif not param_type["raw"] in ["float", "double"]:
                print "unusable_type clang ", param_type

    values["param_count"] = len(params)
    values["params"] = params
    return values

def __map_line_to_values_at_verify(line, index):
    values = {}

    values["index"] = index
    for index, token in enumerate(line.split(" ")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = token
            #values["raw_origin"] = token
        if index == 2:
            values["address"] = token
    return values

def __map_line_to_values_verify(line, index):
    values = {}

    values["index"] = index
    for index, token in enumerate(line.split(";")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = token
            #values["raw_origin"] = token
        if index == 2:
            values["address"] = token
        if index == 3:
            subtokens = token.split(":")
            values["at"] = ("AT" in subtokens)
            values["plt"] = ("PLT" in subtokens)

            values["tags"] = token
        elif index == 4:
            param_count = 0
            params = []
            for (index, sub_token) in enumerate(token.split(" ")[:-1]):
                
                param_type = __decode_padyn_type(sub_token)
                if param_type["usable"]:
                    params += [sub_token]
                elif not param_type["raw"] in ["float", "double"]:
                    print "unusable_type padyn ", param_type

                if not param_type["is_void"]:
                    param_count = index + 1
            values["param_count"] = param_count
            values["params"] = params
            values["return_type"] = token.split(" ")[-1]

        elif index == 6:
            values["block_start"] = token
        elif index == 7:
            values["instr_addr"] = token

    return values

def __analyze_file_line_ground_truth(result, line, index, tag_pre_fn="", tag_pre_cs=""):
    if line.count("<" + tag_pre_fn + "AT>") > 0:
        result = utils.add_values_to_key(result, "address_taken", __map_line_to_values(line, index))
        result = utils.add_values_to_key(result, "call_target", __map_line_to_values(line, index))
    elif line.count("<" + tag_pre_fn + "FN>") > 0:
        result = utils.add_values_to_key(result, "normal_function", __map_line_to_values(line, index))
        result = utils.add_values_to_key(result, "call_target", __map_line_to_values(line, index))
    elif line.count("<" + tag_pre_cs + "CS>") > 0:
        result = utils.add_values_to_key(result, "call_site", __map_line_to_values(line, index))

    return result

def __analyze_file_line_verify(result, line, index):
    if line.count("<AT>") > 0:
        result = utils.add_values_to_key(result, "address_taken", __map_line_to_values_at_verify(line, index))
    elif line.count("<CT>") > 0:
        value = __map_line_to_values_verify(line, index)

        if not utils.is_internal_function(value["origin"]):
            result = utils.add_values_to_key(result, "call_target", value)
    elif line.count("<CS>") > 0:
        value =  __map_line_to_values_verify(line, index)

        if not utils.is_internal_function(value["origin"]):
            result = utils.add_values_to_key(result, "call_site", value)

    return result

def __analyze_file_lines(in_file, analyzer, result):
    index = 0
    for line in in_file:
        result = analyzer(result, line, index)
        index += 1
    return result

def parse_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_ground_truth, {})

def __analyze_file_line_unfiltered_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "", "U")

def parse_unfiltered_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_unfiltered_ground_truth, {})

def __analyze_file_line_machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M", "M")

def parse_machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_machine_ground_truth, {})

def __analyze_file_line_tailcall_machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M", "TM")

def parse_tailcall_machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_tailcall_machine_ground_truth, {})

def __analyze_file_line_call_machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M", "CM")

def parse_call_machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_call_machine_ground_truth, {})

def __analyze_file_line_unfiltered_machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M", "UM")

def parse_unfiltered_machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_unfiltered_machine_ground_truth, {})

def __analyze_file_line_x86machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M86", "M86")

def parse_x86machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_x86machine_ground_truth, {})

def __analyze_file_line_tailcall_x86machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M86", "TM86")

def parse_tailcall_x86machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_tailcall_x86machine_ground_truth, {})

def __analyze_file_line_call_x86machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M86", "CM86")

def parse_call_x86machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_call_x86machine_ground_truth, {})

def __analyze_file_line_unfiltered_x86machine_ground_truth(result, line, index) :
    return __analyze_file_line_ground_truth(result, line, index, "M86", "UM86")

def parse_unfiltered_x86machine_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as f:
        return __analyze_file_lines(f, __analyze_file_line_unfiltered_x86machine_ground_truth, {})

def parse_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify." + name + ".at")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".cs")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".ct")) as f:
        result = __analyze_file_lines(f, __analyze_file_line_verify, result)

    return result
