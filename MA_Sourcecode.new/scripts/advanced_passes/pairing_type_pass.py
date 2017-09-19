from advanced_pass import AdvancedPass as AdvancedPass

class PairingTypePass(AdvancedPass):
    def __init__(self):
        AdvancedPass.__init__(self, "pairing_type")

    def _run_on_target(self, target, padyn, clang):
        pass
