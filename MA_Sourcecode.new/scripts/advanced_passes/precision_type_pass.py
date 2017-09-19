from advanced_pass import AdvancedPass as AdvancedPass

class PrecisionTypePass(AdvancedPass):
    def __init__(self):
        AdvancedPass.__init__(self, "precision_type")

    def _run_on_target(self, target, padyn, clang):
        pass
