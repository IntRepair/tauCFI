from advanced_pass import AdvancedPass as AdvancedPass

class PairingCountPass(AdvancedPass):
    def __init__(self):
        AdvancedPass.__init__(self, "pairing_count")

    def _run_on_target(self, target, padyn, clang):
        pass
