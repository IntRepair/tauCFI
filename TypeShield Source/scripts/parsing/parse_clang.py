import os
import base_functionality.utilities as utils
import parse_file as fparser

def entry_identity(entry):
    return (entry["demang_name"], entry["mang_name"])

class ContextHistory(object):
    def __init__(self):
        self._current = {}
        self._history = []

    def get_context_switch_needed(self, context):
        if "context" in self._current.keys():
            if context != self._current["context"]:
                self._history += [self._current]
                self._current = {}
                self._current["context"] = context
        else:
            self._current = {}
            self._current["context"] = context
        return self._current

    def get_history_finalized(self):
        if "context" in self._current.keys():
            self._history += [self._current]
            self._current = {}        

        return self._history


class BaseParser(ContextHistory):
    def __init__(self, name, path, FN_tag, CS_tag, linker_tag):
        ContextHistory.__init__(self)
        self._name = name
        self._fn_tag = "<" + FN_tag + ">"
        self._fnx_tag = "<" + FN_tag + "X>"
        self._cs_tag = "<" + CS_tag + ">"
        self._linker_tag = linker_tag

        self.previous_cs = ""
        self.index = 0
        self.result = {}

        with file(os.path.join(path, "ground_truth." + name)) as in_file:
            for line in in_file:
                self._analyze_line(line)
                self.index += 1

    def name(self):
        return self._name

    def _add_module_data(self, module, key, value):
        data_dict = self.get_context_switch_needed("module")
        utils.add_values_to_subkey(data_dict, module, key, value)

    def _get_module_data(self, module, key):
        data_dict = self.get_context_switch_needed("module")
        return utils.get_values_for_subkey(data_dict, module, key)

    def _add_module_subdata(self, module, key, subkey, value):
        data_dict = self.get_context_switch_needed("module")
        utils.add_values_to_subkey(data_dict[module], key, subkey, value)

    def _get_module_subdata(self, module, key, subkey):
        data_dict = self.get_context_switch_needed("module")
        return utils.get_values_for_subkey(data_dict[module], key, subkey)

    def _add_binary_data(self, binary, key, value):
        data_dict = self.get_context_switch_needed("binary")
        utils.add_values_to_subkey(data_dict, binary, key, value)

    def _get_binary_data(self, binary, key):
        data_dict = self.get_context_switch_needed("binary")
        return utils.get_values_for_subkey(data_dict, binary, key)

    def _try_add_module_entry(self, entry, index_label, storage_label):
        module = entry["module"]

        if "compiled" in self._get_module_data(module, "status"):
            return

        name_pair = entry_identity(entry)
        if name_pair not in self._get_module_data(module, index_label):
            self._add_module_data(module, storage_label, entry)
            self._add_module_data(module, index_label, name_pair)
            return

        # We ignore double entries that are equivalent
        for existing_entry in self._get_module_data(module, storage_label):
            if name_pair == entry_identity(existing_entry):
                if utils.equal_dicts(entry, existing_entry, ["index", "module"]):
                    return True

        self._add_module_data(module, storage_label + "_double", entry)
        self._add_module_data(module, index_label + "_double", name_pair)

    def _analyze_line(self, line):
        # Parse Module Entry
        if line.count("<MMD>"):
            entry = fparser.map_line_to_values_module(line)
            module = entry["module"]
            if "compiled" not in self._get_module_data(module, "status"):
                self._add_module_data(module, "status", "compiled")

        # Parse Function Entry
        elif line.count(self._fn_tag) > 0:
            entry = fparser.map_line_to_values_clang(line, self.index)

            self._try_add_module_entry(entry, "ct_index", "call_target")
            if "AT" in entry["flags"]:
                self._try_add_module_entry(entry, "at_index", "address_taken")

        # Parse FunctionX Entry
        elif line.count(self._fnx_tag) > 0:
            entry = fparser.map_line_to_values_clang(line, self.index)

            self._try_add_module_entry(entry, "cfn_index", "compiled_function")

        # Parse Callsite Entry
        elif line.count(self._cs_tag) > 0:
            entry = fparser.map_line_to_values_clang(line, self.index)
            module = entry["module"]

            if "compiled" not in self._get_module_data(module, "status"):
                if "ICS" in entry["flags"]:
                    name_pair = entry_identity(entry)
                    if name_pair not in self._get_module_data(module, "cfn_index"):
                        self._add_module_data(module, "call_site", entry)
                        self._add_module_subdata(module, "cs_index", name_pair, entry)
                    else:
                        self._add_module_data(module, "call_site_double", (name_pair, module))

        # Parse Linker Entry
        elif line.count(self._linker_tag) > 0:
            entry = fparser.map_line_to_values_linker(line)

            if "binary" in entry.keys():
                binary = entry["binary"]

                if binary not in self._get_binary_data(binary, "sources"):
                    self._add_binary_data(binary, "sources", entry)
                else:
                    self._add_binary_data(binary, "sources_double", (binary, entry["source"]))


def _update_information(information, module_entry, storage, index):
    # We need to remove the following:
    # A) non trivial doubles found in module during initial parsing
    # B) non trivial doubles found in information
    # C) non trivial doubles found between information and module

    # Simply add doubles for logging, the actual work is done using indices
    inf_double_store = utils.get_values_for_subkey(information, "double_vals", storage)
    inf_double_store += utils.get_values_for_key(module_entry, storage + "_double")
    # Get the information data, as we will modify this first
    inf_index = utils.get_values_for_key(information, index)
    inf_store = utils.get_values_for_key(information, storage)

    # A) non trivial doubles found in module during initial parsing from information
    inf_double_index = utils.get_values_for_subkey(information, "double_vals", index)
    mod_double_index = utils.get_values_for_key(module_entry, index + "_double")
    # do we have doubles in the module data ?
    if mod_double_index != []:
        add_double_index = list(set(inf_double_index + mod_double_index) - set(inf_double_index))
        inf_double_index += add_double_index
        # We have new additions to double index, so check if we need to remove stuff from information
        if add_double_index != []:
            inf_remove_index = list(set(inf_index).intersection(set(add_double_index)))
            # We need to remove stuff from information index and add to information double index
            for elem in inf_remove_index:
                inf_index.remove(elem)
                inf_double_index += [elem]
            # We need to remove stuff from information store and add to information double store
            inf_remove = [x for x in inf_store if entry_identity(x) in inf_remove_index]
            print inf_remove
            for elem in inf_remove:
                inf_store.remove(elem)
                inf_double_store += [elem]

    # B) Remove non trivial doubles found in information from addition
    # get add_index without doubles found in inf_double_index
    add_index = utils.get_values_for_key(module_entry, index)
    add_index = [x for x in add_index if x not in inf_double_index]

    add_store = utils.get_values_for_key(module_entry, storage)
    # Add double entries within add_store to information double store
    inf_double_store += [x for x in add_store if entry_identity(x) in inf_double_index]
    # Get add_store without doubles found in inf_double_index
    add_store = [x for x in add_store if entry_identity(x) not in inf_double_index]

    # C) non trivial doubles found between information and module
    add_double_index = list(set(inf_index).intersection(set(add_index)))
    # we have possible doubles between information index and addition index
    for index_entry in add_double_index:
        # find element1 in add_store
        elements_add = [x for x in add_store if entry_identity(x) == index_entry]
        # find element2 in inf_store
        elements_inf = [x for x in inf_store if entry_identity(x) == index_entry]
        assert(len(elements_add) == 1)
        assert(len(elements_inf) == 1)
        # remove entry from add_index
        add_index.remove(index_entry)
        # remove element1 from add_store
        add_store.remove(elements_add[0])
        # compare via utils.equal_dicts(element1, element2, ["index", "module"])
        if not utils.equal_dicts(elements_add[0], elements_inf[0], ["index", "module"]):
        # not equal =>
        #   remove entry from inf_index
            inf_index.remove(index_entry)
        #   remove element2 from inf_store
            inf_store.remove(elements_inf[0])
        #   add entry to inf_double_index
            inf_double_index += [index_entry]
        #   add [element1, element2] to inf_double_store 
            inf_double_store += [elements_add[0], elements_inf[0]]

    inf_index += add_index
    inf_store += add_store

    return information

def _update_callsite_information(information, module_entry):
    # Get the compiled function data from information
    inf_cfn_double_index = utils.get_values_for_subkey(information, "double_vals", "cfn_index")
    inf_cfn_index = utils.get_values_for_key(information, "cfn_index")

    # Get the compiled function data from module_entry
    mod_cfn_double_index = utils.get_values_for_key(module_entry, "cfn_index_double")
    mod_cfn_index = utils.get_values_for_key(module_entry, "cfn_index")

    storage = "call_site"
    index = "cs_index"

    inf_store = utils.get_values_for_key(information, storage)
    inf_double_store = utils.get_values_for_subkey(information, "double_vals", storage)
    mod_store = utils.get_values_for_key(module_entry, storage)
    mod_double_store = utils.get_values_for_key(module_entry, storage + "_double")

    #inf_index = utils.get_values_for_>key(information, index)
    #inf_double_index = utils.get_values_for_subkey(information, "double_vals", index)
    #mod_index_utils.get_values_for_subkey(module_entry, index)
    #mod_double_index = utils.get_values_for_key(module_entry, index + "_double")

    # A) determine the callsites that should be removed
    rem_inf_cfn_index = set(inf_cfn_double_index).intersection(set(mod_cfn_index + mod_cfn_double_index))

    for cfn_id in rem_inf_cfn_index:
        for elem in utils.pop_values_for_subkey(information, index, cfn_id):
            inf_store.remove(elem)
            inf_double_store += [elem]

    #for elem in [x for x in inf_store if entry_identity(x) in rem_inf_cfn_index]:
    #    inf_store.remove(elem)
    #    inf_double_store += [elem]

    inf_double_store += mod_double_store

    # B) determine the callsites that should be added
    add_inf_cfn_index = set(inf_cfn_index).intersection(set(mod_cfn_index))

    for cfn_id in add_inf_cfn_index:
        add_inf_css = utils.get_values_for_subkey(module_entry, index, cfn_id)

        if len(add_inf_css) > 0:
            inf_index_cfn = utils.get_values_for_subkey(information, index, cfn_id)
            inf_index_cfn += add_inf_css
            inf_store += add_inf_css

    #inf_store += [x for x in mod_store if entry_identity(x) in add_inf_cfn_index]

    # TODO: C) detect any problems (?) discards / doubles / ...
    return information

def _try_find_source_string(source, keys):
    # Attempt to match based on the full path
    source_string = source["source"]
    if source_string not in keys:
        source_string = source["source2"]
        # Full path method not possible ?
        # Try finding based on the file_name alone, if not ambiguous
        if source_string not in keys:
            fname = os.path.basename(source_string)
            source_strings = [x for x in keys if fname == os.path.basename(x)] 
            # Found unique source_string ? 
            if len(source_strings) == 1:
                source_string = source_strings[0]
            # Unable to find the module ? --> Put into Missing
            else:
                return (False, "")
    return (True, source_string)

def _compose_information_backwards(history, binary, index):
    information = {}
    # todo continue here
    sources_cur = history[index][binary]["sources"]
    sources_next = []
    while sources_cur != [] and index > 0:
        for source in sources_cur:
            module_entry = history[index - 1]
            (found, source_string) = _try_find_source_string(source, module_entry.keys())
            # Unable to find the module ? --> Put into Missing
            if not found:
                sources_next += [source]
            # We found it ? --> great, lets update
            else:
                source_data = module_entry[source_string]
                information = _update_information(information, source_data, "address_taken", "at_index")
                #information = _update_information(information, source_data, "call_target", "ct_index")
                information = _update_information(information, source_data, "compiled_function", "cfn_index")
                information = _update_callsite_information(information, source_data)

        sources_cur = sources_next
        sources_next = []
        index -= 2
    # Output missing sources
    for source in sources_cur: 
        print "Not Found ", source["sourinformationce"], " ", source["source2"]
    return information

class GroundTruthParser(BaseParser):
    def __init__(self, path, name):
        BaseParser.__init__(self, name, path, "MFN", "AMCS", "LINK(OBJFILE)")

    def _compose_information(self, binary):
        history = self.get_history_finalized()
        for index, data in enumerate(history):
            if data["context"] == "binary":
                if binary in data.keys():
                    return _compose_information_backwards(history, binary, index)

        raise Exception("Could not find binary " + binary + " in the history")
        return {}

    def get_binary_info(self, binary):
        if binary in self.result.keys():
            return self.result[binary]

        data = self._compose_information(binary)
        self.result[binary] = data
        return data

def parse_ground_truth(path, name):
    parser = GroundTruthParser(path, name)
    return parser.get_binary_info(name)
