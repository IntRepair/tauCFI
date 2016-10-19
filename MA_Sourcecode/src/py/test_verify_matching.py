import csv
import os
import sys

import test_config

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    with open(os.path.join(test_dir, "error_file.O0"), "w") as e0:
        with open(os.path.join(test_dir, "error_file.O1"), "w") as e1:
            with open(os.path.join(test_dir, "error_file.O2"), "w") as e2:
                with open(os.path.join(test_dir, "error_file.O3"), "w") as e3:
                    with open(os.path.join(test_dir, "error_file.Os"), "w") as es:
                        with open(os.path.join(test_dir, "matching.csv"), "w") as csv_file:

                            error_file = {}
                            error_file["O0"] = e0
                            error_file["O1"] = e1
                            error_file["O2"] = e2
                            error_file["O3"] = e3
                            error_file["Os"] = es

                            fieldnames = ["target", "opt", "fn_count", "fn_problem", "at_count", "at_problem", "cs_count", "cs_clang", "cs_padyn"]
                            writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
                            writer.writeheader()

                            for test_target in test_config.configure_targets(argv):
                                result = test_target.verify_matching_all()
                                for opt in result.keys():
                                    (csv_data, problem_string) = result[opt]
                                    error_file[opt].write(problem_string)
                                    csv_data["opt"] = opt
                                    writer.writerow(csv_data)

main(sys.argv[1:])
