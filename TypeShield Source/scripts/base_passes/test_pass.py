import os

class Pass:
    def __init__(self, name, config, args):
        self.name = name
        self.error_messages = []
	self.config = config
	self.args = args
        root_dir = self.config["type_shield.dir"]
        self.test_dir = os.path.join(root_dir, self.config["type_shield.test.dir"])

    def run_on_targets(self, targets):
        self._run_on_targets(targets)

        file_name = "error_file." + self.name
        with open(os.path.join(self.test_dir, file_name), "w") as error_file:
            error_file.writelines(self.error_messages)

    def _run_on_targets(self, targets):
        raise Exception("Implement _run_on_targets Function for your pass!")

    def log_errors(self, messages):
        for message in messages:
            self.log_error(message)

    def log_error(self, message):
        message = "ERROR [pass: " + str(self.name) + "] " + str(message)
        print message
        self.error_messages += [message + "\n"]

    def log_errors_target(self, target, messages):
        for message in messages:
            self.log_error_target(target, message)

    def log_error_target(self, target, message):
        message = "ERROR [pass: " + str(self.name) + " target: " + str(target) + "] " + str(message)
        print message
        self.error_messages += [message + "\n"]

