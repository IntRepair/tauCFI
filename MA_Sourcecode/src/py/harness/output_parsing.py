import os
import harness.utilities as utils

#ngx_atoi;               <(M)FN>;   i8*;                    i64; ; ; ; ; parameter_count 2;return_type i64;
#ngx_sort;               <(M)CS>;   i8*;                    i8*; ; ; ; ; parameter_count 2;return_type i64;
#ngx_clean_old_cycles;   <(M)AT>;   %struct.ngx_event_s*;   ;    ; ; ; ; parameter_count 1;return_type void;

def is_typename(symbol):
    return symbol in ["double", "float", "void", "u8", "i8", "u16", "i16", "u32", "i32", "u64", "i64"]

def __sanitize_symbol(symbol):

    if symbol.endswith("\\000"):
        symbol = symbol[:-4]
    # here we remove the parameters
#    tokens = symbol.split("(")
#    if len(tokens) > 1:
#        to_be_deleted = tokens[-1]
#        tokens = tokens[:-1]
#        right_par_count = to_be_deleted.count(")") - 1
#
#        while right_par_count > 0:
#            to_be_deleted = tokens[-1]
#            tokens = tokens[:-1]
#            right_par_count += to_be_deleted.count(")") - 1
#
#        symbol = "(".join(tokens)

    #here we remove the
    #prefix = []
    #tokens = symbol.split(" ")
    #if len(tokens) > 3 and tokens[0] == "non-virtual" and tokens[1] == "thunk" and tokens[2] == "to":
    #    prefix = tokens[1:3]
    #    tokens = tokens[3:]


#    if is_typename(tokens[0]):
#        tokens = tokens[1:]
#    elif tokens[0].count("<") > 0 and len(tokens) > 1:
#        token = tokens[0]
#        remainder_token = tokens[1:]
#        par_token = [token]
#        left_param_count = token.count("(")
#        right_param_count = token.count(")")
#        left_temp_count = token.count("<")
#        right_temp_count = token.count(">")
#        while left_temp_count > right_temp_count or left_param_count > right_param_count:
#            token = remainder_token[0]
#            remainder_token = remainder_token[1:]
#            par_token += [token]
#            left_param_count += token.count("(")
#            right_param_count += token.count(")")
#            left_temp_count += token.count("<")
#            right_temp_count += token.count(">")
#
#        if len(remainder_token) > 0:
#            tokens = remainder_token
#            #print " ".join(par_token), " <--> ", " ".join(remainder_token)
#    elif len(tokens) > 1:
#        tokens = tokens[1:]
#
#    if "v8::internal::compiler::" == " ".join(prefix + tokens):
#        print
#        print symbol
#        print

#    result = " ".join(prefix + tokens)
    return symbol

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

def _map_line_to_values(line, index):
    values = {}
    params = []

    line = line.strip()
    tokens = line.split(";")

    #values["index"] = index
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

    return_type["raw"] = ""

    for index, token in enumerate(tokens):
        token = token.strip()
        if index == 0:
            values["origin"] = __sanitize_symbol(token)
            #values["raw_origin"] = token
#        elif index == 1:
#            values["tag"] = token
        elif index == 2:
            module_tokens = token.split(" ")
            values["module"] = module_tokens[len(module_tokens) - 1]
        elif index > 2 and (index - 3) < int(values["param_count"]):
            param_type = __decode_type(token)
            if param_type["usable"]:
                param_type["raw"] = ""
                params += [param_type]
            elif not param_type["raw"] in ["float", "double"]:
                print "unusable_type clang ", param_type

    values["param_count"] = len(params)
    values["params"] = params
    return values

def _map_line_to_values_at_verify(line, index):
    values = {}

    #values["index"] = index
    for index, token in enumerate(line.split(";")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = __sanitize_symbol(token)
            #values["raw_origin"] = token
        if index == 2:
            values["address"] = token
    return values

def _map_line_to_values_verify(line, index):
    values = {}

    #values["index"] = index
    for index, token in enumerate(line.split(";")):
        token = token.strip()
#        elif index == 0:
#            values["tag"] = token
        if index == 1:
            values["origin"] = __sanitize_symbol(token)
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
            #print token
            for (index, sub_token) in enumerate(token.split(" ")[:-1]):

                param_type = __decode_padyn_type(sub_token)
                #print param_type
                if param_type["usable"]:
                    params += [param_type]
                elif not param_type["raw"] in ["float", "double"]:
                    print "unusable_type padyn ", param_type
                    print line
                    print

                param_type["raw"] = ""

                if not param_type["is_void"]:
                    param_count = index + 1
            values["param_count"] = param_count
            values["params"] = params
            values["return_type"] = __decode_padyn_type(token.split(" ")[-1])
            values["return_type"]["raw"] = ""

        elif index == 6:
            values["block_start"] = token
        elif index == 7:
            values["instr_addr"] = token

    return values

def _map_line_to_values_linker(line):
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

def _map_line_to_value_module(line):
    entry = {}

    tokens = line.split(";")
    module_name = tokens[0].strip()
    module_path = tokens[2].strip()

    entry["module"] = os.path.join(module_path, module_name)
    return entry

def __analyze_file_line_ground_truth(result, line, index, tag_pre_fn="", tag_pre_cs=""):
    if "fn_index" not in result.keys():
        result["fn_index"] = []
        result["at_index"] = []
        result["ct_index"] = []
        result["cs_final"] = []
        result["cs_prev"] = ""
        result["double_vals"] = {}
        result["double_vals"]["at"] = {}
        result["double_vals"]["fn"] = {}
        result["double_vals"]["ct"] = {}
        result["double_vals"]["cs"] = {}

    if line.count("<" + tag_pre_fn + "AT>") > 0:
        entry = _map_line_to_values(line, index)

        #if "module" not in result.keys():
        #    result["module"] = entry["module"]
        #    result["modules"] = []
        #elif entry["module"] != result["module"]:
        #    if entry["module"] not in result["modules"]:
        #        result["modules"] += [entry["module"]]
        #        result["module"] = entry["module"]
        #    elif entry["origin"] in (_entry["origin"] for _entry in result["call_target"]):
        #        return

        if entry["origin"] not in result["at_index"]:
            utils.add_values_to_key(result, "address_taken", entry)
            utils.add_values_to_key(result, "at_index", entry["origin"])
        else:
            utils.add_values_to_key(result["double_vals"]["at"], entry["origin"], "I")

        if entry["origin"] not in result["ct_index"]:
            utils.add_values_to_key(result, "call_target", entry)
            utils.add_values_to_key(result, "ct_index", entry["origin"])
        else:
            utils.add_values_to_key(result["double_vals"]["ct"], entry["origin"], "I")

    elif line.count("<" + tag_pre_fn + "FN>") > 0:
        entry = _map_line_to_values(line, index)

        #if "module" not in result.keys():
        #    result["module"] = entry["module"]
        #    result["modules"] = []
        #elif entry["module"] != result["module"]:
        #    if entry["module"] not in result["modules"]:
        #        result["modules"] += [entry["module"]]
        #        result["module"] = entry["module"]
        #    elif entry["origin"] in (_entry["origin"] for _entry in result["call_target"]):
        #        return

        if entry["origin"] not in result["fn_index"]:
            utils.add_values_to_key(result, "normal_function", entry)
            utils.add_values_to_key(result, "fn_index", entry["origin"])
        else:
            utils.add_values_to_key(result["double_vals"]["fn"], entry["origin"], "I")

        if entry["origin"] not in result["ct_index"]:
            utils.add_values_to_key(result, "call_target", entry)
            utils.add_values_to_key(result, "ct_index", entry["origin"])
        else:
            utils.add_values_to_key(result["double_vals"]["ct"], entry["origin"], "I")

    elif line.count("<" + tag_pre_cs + "CS>") > 0:
        entry = _map_line_to_values(line, index)

        #if "module" not in result.keys():
        #    result["module"] = entry["module"]
        #    result["modules"] = []
        #elif entry["module"] != result["module"]:
        #    if entry["module"] not in result["modules"]:
        #        result["modules"] += [entry["module"]]
        #        result["module"] = entry["module"]
        #    elif entry["origin"] in (_entry["origin"] for _entry in result["call_site"]):
        #        return

        if result["cs_prev"] != entry["origin"] and result["cs_prev"] != "":
            utils.add_values_to_key(result, "cs_final", result["cs_prev"])
            result["cs_prev"] = ""

        if result["cs_prev"] == entry["origin"]:
            utils.add_values_to_key(result, "call_site", entry)
        elif result["cs_prev"] == "" and entry["origin"] not in result["cs_final"]:
            result["cs_prev"] = entry["origin"]
            utils.add_values_to_key(result, "call_site", entry)
        else:
            utils.add_values_to_key(result["double_vals"]["cs"], entry["origin"], "I")

    return result

def __analyze_file_line_verify(result, line, index):
    if "at_index" not in result.keys():
        result["at_index"] = []
        result["ct_index"] = []
        result["double_vals"] = {}
        result["double_vals"]["at"] = {}
        result["double_vals"]["fn"] = {}
        result["double_vals"]["ct"] = {}
        result["double_vals"]["cs"] = {}


    if line.count("<AT>") > 0:
        entry = _map_line_to_values_at_verify(line, index)

        if entry["origin"] not in result["at_index"]:
            utils.add_values_to_key(result, "address_taken", entry)
            utils.add_values_to_key(result, "at_index", entry["origin"])
        else:
            result["double_vals"]["at"][entry["origin"]] = "I"

    elif line.count("<CT>") > 0:
        entry = _map_line_to_values_verify(line, index)

        if not utils.is_internal_function(entry["origin"]):
            if entry["origin"] not in result["ct_index"]:
                utils.add_values_to_key(result, "call_target", entry)
                utils.add_values_to_key(result, "ct_index", entry["origin"])
        else:
            result["double_vals"]["ct"][entry["origin"]] = "I"

    elif line.count("<CS>") > 0:
        value =  _map_line_to_values_verify(line, index)

        if not utils.is_internal_function(value["origin"]):
            utils.add_values_to_key(result, "call_site", value)

    return result

def __analyze_file_lines(in_file, analyzer, result):
    index = 0
    for line in in_file:
        analyzer(result, line, index)
        index += 1
    return result

#def __analyze_file_line_unfiltered_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "", "U")

#def __analyze_file_line_machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M", "M")

#def __analyze_file_line_tailcall_machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M", "TM")

#def __analyze_file_line_call_machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M", "CM")

#def __analyze_file_line_unfiltered_machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M", "UM")

#def __analyze_file_line_x86machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M86", "M86")

#def __analyze_file_line_tailcall_x86machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M86", "TM86")

#def __analyze_file_line_call_x86machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M86", "CM86")

#def __analyze_file_line_unfiltered_x86machine_ground_truth(result, line, index) :
#    return __analyze_file_line_ground_truth(result, line, index, "M86", "UM86")

def __analyze_file_line_augment_machine_ground_truth(result, line, index):
    return __analyze_file_line_ground_truth(result, line, index, "M", "AM")

def parse_ground_truth(path, name):
    with file(os.path.join(path, "ground_truth." + name)) as fun:
        result = __analyze_file_lines(fun, __analyze_file_line_augment_machine_ground_truth, {})

        return result

def parse_count_safe_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify.count_safe." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.count_safe." + name + ".cs")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.count_safe." + name + ".ct")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

def parse_verify_basic(path, name):
    result = {}
    with file(os.path.join(path, "verify." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".cs")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify." + name + ".ct")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

def parse_verify(path, name, tag):
    result = {}

    with file(os.path.join(path, "verify." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    file_path = os.path.join(path, "verify." + tag + "." + name + ".ct")
    if not os.path.isfile(file_path):
        file_path = os.path.join(path, "verify." + name + ".ct")

    with file(file_path) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    file_path = os.path.join(path, "verify." + tag + "." + name + ".cs")
    if not os.path.isfile(file_path):
        file_path = os.path.join(path, "verify." + name + ".cs")

    with file(file_path) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

def parse_count_prec_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify.count_prec." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.count_prec." + name + ".cs")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.count_prec." + name + ".ct")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

def parse_type_safe_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify.type_exp2." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.type_exp3." + name + ".cs")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.type_exp2." + name + ".ct")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

def parse_type_prec_verify(path, name):
    result = {}
    with file(os.path.join(path, "verify.type_exp6." + name + ".at")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.type_exp6." + name + ".cs")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    with file(os.path.join(path, "verify.type_exp6." + name + ".ct")) as fun:
        __analyze_file_lines(fun, __analyze_file_line_verify, result)

    return result

class BaseParser:
    def __init__(self, name, path, AT_tag, FN_tag, CS_tag, linker_tag):
        self._name = name
        self._at_tag = AT_tag
        self._fn_tag = FN_tag
        self._cs_tag = CS_tag
        self._linker_tag = linker_tag
        self._file_path = os.path.join(path, "ground_truth." + name)

        self.previous_cs = ""
        self.index = 0

    def name(self):
        return self._name

    def at_tag(self):
        return self._at_tag

    def fn_tag(self):
        return self._fn_tag

    def cs_tag(self):
        return self._cs_tag

    def linker_tag(self):
        return self._linker_tag

    def file_path(self):
        return self._file_path

    def _add_data(self, data_dict, key, subkey, value):
        if key not in data_dict.keys():
            data_dict[key] = {}
        utils.add_values_to_key(data_dict[key], subkey, value)

    def _get_data(self, data_dict, key, subkey):
        if key not in data_dict.keys():
            data_dict[key] = {}

        return utils.get_values_for_key(data_dict[key], subkey)

    def _initialize_analysis(self):
        raise NotImplementedError("_initialize_analysis")

    def _finalize_analysis(self):
        raise NotImplementedError("_initialize_analysis")

    def _analyze_line(self, line):
        raise NotImplementedError("_analyze_line")

    def _analyze(self):
        with file(self.file_path()) as in_file:
            self._initialize_analysis()

            self.index = 0
            self.previous_cs = ""

            for line in in_file:
                self._analyze_line(line)
                self.index += 1

            self._finalize_analysis()

    def _try_analyze_compiler(self, line, add_module_data_fn, get_module_data_fn):
        if line.count("<MMD>"):
            entry = _map_line_to_value_module(line)
            add_module_data_fn(entry["module"], "compiled", "")

        elif line.count(self.at_tag()) > 0:
            entry = _map_line_to_values(line, self.index)

            if entry["origin"] not in get_module_data_fn(entry["module"], "at_index"):
                add_module_data_fn(entry["module"], "address_taken", entry)
                add_module_data_fn(entry["module"], "at_index", entry["origin"])
            else:
                add_module_data_fn(entry["module"], "at_index_double", entry["origin"])


            if entry["origin"] not in get_module_data_fn(entry["module"], "ct_index"):
                add_module_data_fn(entry["module"], "call_target", entry)
                add_module_data_fn(entry["module"], "ct_index", entry["origin"])
            else:
                add_module_data_fn(entry["module"], "ct_index_double", entry["origin"])

            return True

        # Parse Fuction Entry
        elif line.count(self.fn_tag()) > 0:
            entry = _map_line_to_values(line, self.index)

            if entry["origin"] not in get_module_data_fn(entry["module"], "fn_index"):
                add_module_data_fn(entry["module"], "normal_function", entry)
                add_module_data_fn(entry["module"], "fn_index", entry["origin"])
            else:
                add_module_data_fn(entry["module"], "fn_index_double", entry["origin"])

            if entry["origin"] not in get_module_data_fn(entry["module"], "ct_index"):
                add_module_data_fn(entry["module"], "call_target", entry)
                add_module_data_fn(entry["module"], "ct_index", entry["origin"])
            else:
                add_module_data_fn(entry["module"], "ct_index_double", entry["origin"])

            return True

        # Parse Callsite Entry
        elif line.count(self.cs_tag()) > 0:
            entry = _map_line_to_values(line, self.index)

            if self.previous_cs != entry["origin"] and self.previous_cs != "":
                add_module_data_fn(entry["module"], "cs_final", self.previous_cs)
                self.previous_cs = ""

            if self.previous_cs == entry["origin"]:
                add_module_data_fn(entry["module"], "call_site", entry)
            elif self.previous_cs == "" and entry["origin"] not in get_module_data_fn(entry["module"], "cs_final"):
                self.previous_cs = entry["origin"]
                add_module_data_fn(entry["module"], "call_site", entry)
            else:
                add_module_data_fn(entry["module"], "call_site_double", entry["origin"])

            return True

        return False

    def _try_analyze_linker(self, line, add_binary_data_fn, get_binary_data_fn):
        if line.count(self.linker_tag()) > 0:
            entry = _map_line_to_values_linker(line)

            if "binary" not in entry.keys():
                return

            if entry["binary"] not in get_binary_data_fn(entry["binary"], "sources"):
                add_binary_data_fn(entry["binary"], "sources", entry)
            else:
                add_binary_data_fn(entry["binary"], "sources_double", entry["source"])

class GroundTruthParser(BaseParser):
    def __init__(self, path, name):
        BaseParser.__init__(self, name, path, "<MAT>", "<MFN>", "<AMCS>", "LINK(OBJFILE)")

        self.current_dict = {}
        self.history = []
        self.result = {}

        self._analyze()

    def _set_new_context(self, type_name):
        if "dict_type" in self.current_dict.keys():
            self.history += [self.current_dict]
            self.current_dict = {}
            self.current_dict["dict_type"] = type_name
        elif [] == self.current_dict.keys():
            self.current_dict["dict_type"] = type_name
        else:
            raise Exception("Something wrent wrong in the context switching!")

    def _add_module_data(self, module, key, value):
        self._add_data(self.current_dict, module, key, value)

    def _get_module_data(self, module, key):
        if self.current_dict["dict_type"] != "compile":
            self._set_new_context("compile")

        return self._get_data(self.current_dict, module, key)

    def _add_binary_data(self, binary, key, value):
        self._add_data(self.current_dict, binary, key, value)

    def _get_binary_data(self, binary, key):
        if self.current_dict["dict_type"] != "link":
            self._set_new_context("link")

        return self._get_data(self.current_dict, binary, key)

    def _initialize_analysis(self):
        self.result = {}
        self._set_new_context("compile")

    def _finalize_analysis(self):
        self.history += [self.current_dict]
        self.current_dict = {}

    def _analyze_line(self, line):
        if not self._try_analyze_compiler(line, self._add_module_data, self._get_module_data):
            self._try_analyze_linker(line, self._add_binary_data, self._get_binary_data)

    def _update_information(self, information, binary_data, module_entry):
        for source in binary_data:
            source_string = source["source"]
            if source_string not in module_entry.keys():
                source_string = source["source2"]

            if source_string  not in module_entry.keys():
                fname = os.path.basename(source_string)
                source_strings = []
                for x in module_entry.keys():
                    if fname == os.path.basename(x):
                        source_strings += [x]

                if len(source_strings) == 1:
                    source_string = source_strings[0]

            if source_string  not in module_entry.keys():
                information["missing"] += [source]
            else:
                source_data = module_entry[source_string]

                double_index = utils.get_values_for_key(source_data, "at_index_double")
                if double_index != []:
                    print "at_index_double in ", source_string, ": ", double_index

                double_index = utils.get_values_for_key(source_data, "ct_index_double")
                if double_index != []:
                    print "ct_index_double in ", source_string, ": ", double_index

                double_index = utils.get_values_for_key(source_data, "fn_index_double")
                if double_index != []:
                    print "fn_index_double in ", source_string, ": ", double_index

                double_index = utils.get_values_for_key(source_data, "call_site_double")
                if double_index != []:
                    print "call_site_double in ", source_string, ": ", double_index

                addition = []
                addition_index = []

                for new_entry in utils.get_values_for_key(source_data, "address_taken"):
                    is_addition = True

                    if new_entry["origin"] in information["cache_at"].keys():
                        is_addition = False
                        if not utils.equal_dicts(new_entry, information["cache_at"][new_entry["origin"]], ["index", "module"]):
                            information["double_vals"]["at"][new_entry["origin"]] = "I"
                            print "inter module double at_index: "
                            print "Old entry", information["cache_at"][new_entry["origin"]]
                            print "New entry", new_entry
                            print

                    if is_addition:
                        addition_index += [new_entry["origin"]]
                        addition += [new_entry]
                        information["cache_at"][new_entry["origin"]] = new_entry

                information["at_index"] += addition_index
                information["address_taken"] += addition

                addition = []
                addition_index = []

                for new_entry in utils.get_values_for_key(source_data, "call_target"):
                    is_addition = True

                    if new_entry["origin"] in information["cache_ct"].keys():
                        is_addition = False
                        if not utils.equal_dicts(new_entry, information["cache_ct"][new_entry["origin"]], ["index", "module"]):
                            information["double_vals"]["ct"][new_entry["origin"]] = "I"
                            print "inter module double ct_index: "
                            print "Old entry", information["cache_ct"][new_entry["origin"]]
                            print "New entry", new_entry
                            print

                    if is_addition:
                        addition_index += [new_entry["origin"]]
                        addition += [new_entry]
                        information["cache_ct"][new_entry["origin"]] = new_entry

                information["ct_index"] += addition_index
                information["call_target"] += addition


                addition = []
                addition_index = []

                for new_entry in utils.get_values_for_key(source_data, "normal_function"):
                    is_addition = True

                    if new_entry["origin"] in information["cache_fn"].keys():
                        is_addition = False
                        if not utils.equal_dicts(new_entry, information["cache_fn"][new_entry["origin"]], ["index", "module"]):
                            information["double_vals"]["fn"][new_entry["origin"]] = "I"
                            print "inter module double fn_index: "
                            print "Old entry", information["cache_fn"][new_entry["origin"]]
                            print "New entry", new_entry
                            print

                    if is_addition:
                        addition_index += [new_entry["origin"]]
                        addition += [new_entry]
                        information["cache_fn"][new_entry["origin"]] = new_entry

                information["fn_index"] += addition_index
                information["normal_function"] += addition

                information["call_site"] += utils.get_values_for_key(source_data, "call_site")

        return information
    
    def _collate_information_backwards(self, binary, index):
        information = {}

        information["ct_index"] = []
        information["at_index"] = []
        information["fn_index"] = []
        information["call_target"] = []
        information["address_taken"] = []
        information["normal_function"] = []
        information["call_site"] = []
        information["missing"] = self.history[index][binary]["sources"]
        information["cache_at"] = {}
        information["cache_ct"] = {}
        information["cache_fn"] = {}
        information["double_vals"] = {}
        information["double_vals"]["at"] = {}
        information["double_vals"]["fn"] = {}
        information["double_vals"]["ct"] = {}
        information["double_vals"]["cs"] = {}

        while (not information["missing"] == {}) and index > 0:
            sources = information["missing"]
            information["missing"] = []
            information = self._update_information(information, sources, self.history[index - 1])
            index -= 2

        for source in information["missing"]: 
            print "Not Found ", source["source"], " ", source["source2"]

        return information

    def _collate_information(self, binary):
        for index, data in enumerate(self.history):
            if data["dict_type"] == "link":
                if binary in data.keys():
                    return self._collate_information_backwards(binary, index)

        return {}

    def get_binary_info(self, binary):
        if binary in self.result.keys():
            return self.result[binary]
        else:
            data = self._collate_information(binary)
            self.result[binary] = data
            return data
