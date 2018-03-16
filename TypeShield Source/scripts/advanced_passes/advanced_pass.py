import os
import csv

import base_functionality.utilities as utils

from base_passes.test_pass import Pass

from parsing.parse_verify import parse_verify_basic as parse_verify
from parsing.parse_clang import parse_ground_truth as parse_ground_truth

class AdvancedPass(Pass):
    def __init__(self, name, config, args):
        Pass.__init__(self, name, config, args)
        self.csv_data = {}
        self.csv_fields = {}
        self.csv_dicts = {}

    def _run_on_target(self, target, padyn, clang):
        raise "Not Implemented Yet"

    def _run_on_target_parse(self, target):
        print "target", target.prefix, target.binary_name
        padyn = parse_verify(target.prefix, target.binary_name)
        clang = parse_ground_truth(target.prefix, target.binary_name)
        return self._run_on_target(target, padyn, clang)

    def csv_set_define(self, csv_name, csv_fields):
        self.csv_fields[csv_name] = csv_fields

    def csv_output_row(self, csv_name, csv_data):
        utils.add_values_to_key(self.csv_dicts, csv_name, csv_data)

        for key in csv_data.keys():
            utils.add_values_to_subkey(self.csv_data, csv_name, key, csv_data[key])

    def _run_on_targets(self, targets):
        csv_datas = []

        for target in targets:
            self._run_on_target_parse(target)

        for csv_name in self.csv_fields.keys():
            with open(os.path.join(self.test_dir, csv_name + ".csv"), "w") as csv_file: 
                writer = csv.DictWriter(csv_file, fieldnames=self.csv_fields[csv_name]) 
                writer.writeheader()
                writer.writerows(self.csv_dicts[csv_name])
