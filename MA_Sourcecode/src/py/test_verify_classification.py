import csv
import os
import sys

import test_config

def gen_pct_string(value):
    return str(float(int(float(value) * float(10000))) / float(100))

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    with open(os.path.join(test_dir, "error_file_class.O0"), "w") as e0:
        with open(os.path.join(test_dir, "error_file_class.O1"), "w") as e1:
            with open(os.path.join(test_dir, "error_file_class.O2"), "w") as e2:
                with open(os.path.join(test_dir, "error_file_class.O3"), "w") as e3:
                    with open(os.path.join(test_dir, "error_file_class.Os"), "w") as es:
                        with open(os.path.join(test_dir, "classification_cs.csv"), "w") as cs_csv_file:
                            with open(os.path.join(test_dir, "classification_ct.csv"), "w") as ct_csv_file:
                                with open(os.path.join(test_dir, "compound.csv"), "w") as compound_csv_file:

                                    error_file = {}
                                    error_file["O0"] = e0
                                    error_file["O1"] = e1
                                    error_file["O2"] = e2
                                    error_file["O3"] = e3
                                    error_file["Os"] = es

                                    cs_fieldnames = ["target", "opt", "#cs", "problems", "+0", "+1", "+2", "+3", "+4", "+5", "+6", "non-void-ok", "non-void-problem"]
                                    ct_fieldnames = ["target", "opt", "#ct", "problems", "-0", "-1", "-2", "-3", "-4", "-5", "-6", "void-ok", "void-problem"]
                                    compound_fieldnames = ["opt", "target"]
                                    compound_fieldnames += ["#cs", "cs args (perfect%)", "cs args (problem%)", "cs non-void (correct%)", "cs non-void (problem%)"]
                                    compound_fieldnames += ["#ct", "ct args (perfect%)", "ct args (problem%)", "ct void (correct%)", "ct void (problem%)"]

                                    cs_writer = csv.DictWriter(cs_csv_file, fieldnames=cs_fieldnames)
                                    cs_writer.writeheader()
                                    ct_writer = csv.DictWriter(ct_csv_file, fieldnames=ct_fieldnames)
                                    ct_writer.writeheader()
                                    compound_writer = csv.DictWriter(compound_csv_file, fieldnames=compound_fieldnames)
                                    compound_writer.writeheader()

                                    for test_target in test_config.configure_targets(argv):
                                        result = test_target.verify_classification_all()

                                        for opt in result.keys():
                                            (cs_dict, ct_dict, problem_string) = result[opt]
                                            error_file[opt].write(problem_string)
                                            cs_dict["opt"] = opt
                                            ct_dict["opt"] = opt
                                            cs_writer.writerow(cs_dict)
                                            ct_writer.writerow(ct_dict)

                                            compound_dict = {}
                                            compound_dict["opt"] = opt
                                            compound_dict["target"] = cs_dict["target"]
                                            compound_dict["#cs"] = cs_dict["#cs"]
                                            if 0 == int(cs_dict["#cs"]):
                                                compound_dict["cs args (perfect%)"] = "-"
                                                compound_dict["cs args (problem%)"] = "-"
                                                compound_dict["cs non-void (correct%)"] = "-"
                                                compound_dict["cs non-void (problem%)"] = "-"
                                            else:
                                                compound_dict["cs args (perfect%)"] = str(cs_dict["+0"]) + " (" + gen_pct_string(float(cs_dict["+0"]) / float(cs_dict["#cs"])) + "%)"
                                                compound_dict["cs args (problem%)"] = str(cs_dict["problems"]) + " (" + gen_pct_string(float(cs_dict["problems"]) / float(cs_dict["#cs"])) + "%)"
                                                compound_dict["cs non-void (correct%)"] = str(cs_dict["non-void-ok"]) + " (" + gen_pct_string(float(cs_dict["non-void-ok"]) / float(cs_dict["#cs"])) + "%)"
                                                compound_dict["cs non-void (problem%)"] = str(cs_dict["non-void-problem"]) + " (" + gen_pct_string(float(cs_dict["non-void-problem"]) / float(cs_dict["#cs"])) + "%)"

                                            compound_dict["#ct"] = ct_dict["#ct"]
                                            if 0 == int(ct_dict["#ct"]):
                                                compound_dict["ct args (perfect%)"] = "-"
                                                compound_dict["ct args (problem%)"] = "-"
                                                compound_dict["ct void (correct%)"] = "-"
                                                compound_dict["ct void (problem%)"] = "-"
                                            else:
                                                compound_dict["ct args (perfect%)"] = str(ct_dict["-0"]) + " (" + gen_pct_string(float(ct_dict["-0"]) / float(ct_dict["#ct"])) + "%)"
                                                compound_dict["ct args (problem%)"] = str(ct_dict["problems"]) + " (" + gen_pct_string(float(ct_dict["problems"]) / float(ct_dict["#ct"])) + "%)"
                                                compound_dict["ct void (correct%)"] = str(ct_dict["void-ok"]) + " (" + gen_pct_string(float(ct_dict["void-ok"]) / float(ct_dict["#ct"])) + "%)"
                                                compound_dict["ct void (problem%)"] = str(ct_dict["void-problem"]) + " (" + gen_pct_string(float(ct_dict["void-problem"]) / float(ct_dict["#ct"])) + "%)"

                                            compound_writer.writerow(compound_dict)
                                            

main(sys.argv[1:])
