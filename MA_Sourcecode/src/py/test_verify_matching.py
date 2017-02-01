import csv
import os
import sys

import test_config

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    with open(os.path.join(test_dir, "error_file_matching.O0"), "w") as e0:
        with open(os.path.join(test_dir, "error_file_matching.O1"), "w") as e1:
            with open(os.path.join(test_dir, "error_file_matching.O2"), "w") as e2:
                with open(os.path.join(test_dir, "error_file_matching.O3"), "w") as e3:
                    with open(os.path.join(test_dir, "error_file_matching.Os"), "w") as es:
                        with open(os.path.join(test_dir, "matching.O0.csv"), "w") as csv_file0:
                            with open(os.path.join(test_dir, "matching.O1.csv"), "w") as csv_file1:
                                with open(os.path.join(test_dir, "matching.O2.csv"), "w") as csv_file2:
                                    with open(os.path.join(test_dir, "matching.O3.csv"), "w") as csv_file3:
                                        with open(os.path.join(test_dir, "matching.O4.csv"), "w") as csv_files:

                                            error_file = {}
                                            error_file["O0"] = e0
                                            error_file["O1"] = e1
                                            error_file["O2"] = e2
                                            error_file["O3"] = e3
                                            error_file["Os"] = es

                                            fieldnames = ["target", "opt", "fn", "fn not in clang", "fn not in padyn", "at", "at not in clang", "at not in padyn", "cs", "cs discarded clang", "cs discarded padyn"]
                                            opt_writer = {}
                                            writer = csv.DictWriter(csv_file0, fieldnames=fieldnames)
                                            writer.writeheader()
                                            opt_writer["O0"] = writer
                                            writer = csv.DictWriter(csv_file1, fieldnames=fieldnames)
                                            writer.writeheader()
                                            opt_writer["O1"] = writer
                                            writer = csv.DictWriter(csv_file2, fieldnames=fieldnames)
                                            writer.writeheader()
                                            opt_writer["O2"] = writer
                                            writer = csv.DictWriter(csv_file3, fieldnames=fieldnames)
                                            writer.writeheader()
                                            opt_writer["O3"] = writer
                                            writer = csv.DictWriter(csv_files, fieldnames=fieldnames)
                                            writer.writeheader()
                                            opt_writer["Os"] = writer

                                            for test_target in test_config.configure_targets(argv):
                                                result = test_target.verify_matching_all()
                                                for opt in result.keys():
                                                    (csv_data, problem_string) = result[opt]
                                                    error_file[opt].write(problem_string)
                                                    csv_data["opt"] = opt
                                                    opt_writer[opt].writerow(csv_data)

main(sys.argv[1:])
