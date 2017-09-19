import os

class Pass:
    def __init__(self, name, config, args):
        self.name = name
        self.error_messages = []
	self.config = config
	self.args = args

    def run_on_targets(self, targets):
        self._run_on_targets(targets)

	root_dir = self.config["type_shield.dir"]
        test_dir = os.path.join(root_dir, self.config["type_shield.test.dir"])
        file_name = "error_file." + self.name + ".O2"
        with open(os.path.join(test_dir, file_name), "w") as error_file:
            for error_message in self.error_messages:
                error_file.write(error_message)

    def _run_on_targets(self, targets):
        raise Exception("Implement _run_on_targets Function for your pass!")

    def log_errors(self, messages):
        for message in messages:
            self.log_error(message)

    def log_error(self, message):
        message = "ERROR (pass: " + str(name) + ")" + str(message)
        print message
        self.error_messages += [message]

#def __run_in_test_environment(argv, name, csv_defs, fn, error_files, csv_writers):
#    csv_def = csv_defs[0]
#    csv_defs = csv_defs[1:]
#    csv_name = csv_def[0]
#    csv_fields = csv_def[1]
#    test_dir = get_test_dir(argv)
#    with open(os.path.join(test_dir, csv_name + ".O2.csv"), "w") as csv_file2:
#        __csv_writers = {}
#        writer = csv.DictWriter(csv_file2, fieldnames=csv_fields)
#        writer.writeheader()
#        __csv_writers["O2"] = writer
#        csv_writers[csv_name] = __csv_writers
#
#        if len(csv_defs) > 0:
#            __run_in_test_environment(argv, name, csv_defs, fn, error_files, csv_writers)
#        else:
#            fn(argv, error_files, csv_writers)
#
#def run_in_test_environment(argv, name, csv_defs, fn):
#    test_dir = get_test_dir(argv)
#    with open(os.path.join(test_dir, "error_file" + name + ".O2"), "w") as e2:
#        error_files = {}
#        error_files["O2"] = e2
#        __run_in_test_environment(argv, name, csv_defs, fn, error_files, {})
#
