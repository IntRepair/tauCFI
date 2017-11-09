import os

from base_passes.test_pass import Pass

from parsing.parse_verify import parse_verify_basic as parse_verify
from parsing.parse_clang import parse_ground_truth as parse_ground_truth

class AdvancedPass(Pass):
    def __init__(self, name, config, args):
        Pass.__init__(self, name, config, args)

    def _run_on_target(self, target, padyn, clang):
        raise "Not Implemented Yet"

    def _run_on_target_parse(self, target):
        print "target", target.prefix, target.binary_name
	padyn = parse_verify(target.prefix, target.binary_name)
        clang = parse_ground_truth(target.prefix, target.binary_name)
        self._run_on_target(target, padyn, clang)

    def _run_on_targets(self, targets):
        for target in targets:
            self._run_on_target_parse(target)
