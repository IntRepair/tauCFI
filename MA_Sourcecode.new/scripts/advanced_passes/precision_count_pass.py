from advanced_pass import AdvancedPass as AdvancedPass

class PrecisionCountPass(AdvancedPass):
    def __init__(self):
        AdvancedPass.__init__(self, "precision_count")

    def _run_on_target(self, target, padyn, clang):
        pass
