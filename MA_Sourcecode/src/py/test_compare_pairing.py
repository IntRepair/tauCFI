import csv
import os
import sys

import test_config

def gen_pct_string(value):
    return str(float(int(float(value) * float(10000))) / float(100))

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    with open(os.path.join(test_dir, "pairing_compares.csv"), "w") as policy_csv_file:
        policy_fieldnames = ["target", "opt", "policy", "0", "1", "2", "3", "4", "5", "6", "summary"]
        policy_writer = csv.DictWriter(policy_csv_file, fieldnames=policy_fieldnames)
        policy_writer.writeheader()

        for test_target in test_config.configure_targets(argv):
            result = test_target.compare_pairing_all()

            for opt in result.keys():
                (policy_dicts, problem_string) = result[opt]
                for policy_dict in policy_dicts:
                    policy_dict["opt"] = opt
                    policy_writer.writerow(policy_dict)

main(sys.argv[1:])
