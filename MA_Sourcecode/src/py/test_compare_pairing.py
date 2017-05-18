import csv
import os
import sys

import test_config

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    policy_fieldnames = ["target", "opt", "policy"]
    policy_fieldnames += ["0 cs", "0 cts"]
    policy_fieldnames += ["1 cs", "1 cts"]
    policy_fieldnames += ["2 cs", "2 cts"]
    policy_fieldnames += ["3 cs", "3 cts"]
    policy_fieldnames += ["4 cs", "4 cts"]
    policy_fieldnames += ["5 cs", "5 cts"]
    policy_fieldnames += ["6 cs", "6 cts"]
    policy_fieldnames += ["summary cs", "summary cts"]

    with open(os.path.join(test_dir, "pairing_compares.csv"), "w") as policy_csv_file:
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
