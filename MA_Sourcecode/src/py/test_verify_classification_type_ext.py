import csv
import os
import sys

import test_config

def gen_pct_string(value):
    return str(float(int(float(value) * float(10000))) / float(100))

def count_analysis_exp1(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp1")

def count_analysis_exp2(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp2")

def count_analysis_exp3(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp3")

def count_analysis_exp4(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp4")

def count_analysis_exp5(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp5")

def count_analysis_exp6(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp6")

def count_analysis_exp7(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp7")

def count_analysis_exp8(argv, error_files, csv_writers):
    count_analysis(argv, error_files, csv_writers, "type_exp8")

def count_analysis(argv, error_files, csv_writers, tag):
    compound_writers = csv_writers["classification_comp2." + tag]

    for test_target in test_config.configure_targets(argv):
        result = test_target.verify_classification_type_ext_all(tag)

        for opt in result.keys():
            (cs_dict, ct_dict, problem_string) = result[opt]
            error_files[opt].write(problem_string)

            compound_dict = {}
            compound_dict["opt"] = opt
            compound_dict["target"] = cs_dict["target"]
            compound_dict["cs"] = cs_dict["cs"]
            if 0 == int(cs_dict["cs"]):
                compound_dict["cs args perfect"] = "-"
                compound_dict["cs args perfect pct"] = "-"
                compound_dict["cs args problem"] = "-"
                compound_dict["cs args problem pct"] = "-"
                compound_dict["cs non-void correct"] = "-"
                compound_dict["cs non-void correct pct"] = "-"
                compound_dict["cs non-void problem"] = "-"
                compound_dict["cs non-void problem pct"] = "-"
            else:
                compound_dict["cs args perfect"] = str(cs_dict["perfect"])
                compound_dict["cs args perfect pct"] = gen_pct_string(float(cs_dict["perfect"]) / float(cs_dict["cs"]))
                compound_dict["cs args problem"] = str(cs_dict["problems"])
                compound_dict["cs args problem pct"] = gen_pct_string(float(cs_dict["problems"]) / float(cs_dict["cs"]))
                compound_dict["cs non-void correct"] = str(cs_dict["non-void-ok"])
                compound_dict["cs non-void correct pct"] = gen_pct_string(float(cs_dict["non-void-ok"]) / float(cs_dict["cs"]))
                compound_dict["cs non-void problem"] = str(cs_dict["non-void-problem"])
                compound_dict["cs non-void problem pct"] = gen_pct_string(float(cs_dict["non-void-problem"]) / float(cs_dict["cs"]))

            compound_dict["ct"] = ct_dict["ct"]
            if 0 == int(ct_dict["ct"]):
                compound_dict["ct args perfect"] = "-"
                compound_dict["ct args perfect pct"] = "-"
                compound_dict["ct args problem"] = "-"
                compound_dict["ct args problem pct"] = "-"
                compound_dict["ct void correct"] = "-"
                compound_dict["ct void correct pct"] = "-"
                compound_dict["ct void problem"] = "-"
                compound_dict["ct void problem pct"] = "-"
            else:
                compound_dict["ct args perfect"] = str(ct_dict["perfect"])
                compound_dict["ct args perfect pct"] = gen_pct_string(float(ct_dict["perfect"]) / float(ct_dict["ct"]))
                compound_dict["ct args problem"] = str(ct_dict["problems"])
                compound_dict["ct args problem pct"] = gen_pct_string(float(ct_dict["problems"]) / float(ct_dict["ct"]))
                compound_dict["ct void correct"] = str(ct_dict["void-ok"])
                compound_dict["ct void correct pct"] = gen_pct_string(float(ct_dict["void-ok"]) / float(ct_dict["ct"]))
                compound_dict["ct void problem"] = str(ct_dict["void-problem"])
                compound_dict["ct void problem pct"] = gen_pct_string(float(ct_dict["void-problem"]) / float(ct_dict["ct"]))

            compound_writers[opt].writerow(compound_dict)  


def main(argv):
    comp_fieldnames = ["opt", "target"]

    comp_fieldnames += ["cs"]
    comp_fieldnames += ["cs args perfect"]
    comp_fieldnames += ["cs args perfect pct"]
    comp_fieldnames += ["cs args problem"]
    comp_fieldnames += ["cs args problem pct"]
    comp_fieldnames += ["cs non-void correct"]
    comp_fieldnames += ["cs non-void correct pct"]
    comp_fieldnames += ["cs non-void problem"]
    comp_fieldnames += ["cs non-void problem pct"]
    comp_fieldnames += ["ct"]
    comp_fieldnames += ["ct args perfect"]
    comp_fieldnames += ["ct args perfect pct"]
    comp_fieldnames += ["ct args problem"]
    comp_fieldnames += ["ct args problem pct"]
    comp_fieldnames += ["ct void correct"]
    comp_fieldnames += ["ct void correct pct"]
    comp_fieldnames += ["ct void problem"]
    comp_fieldnames += ["ct void problem pct"]

    csv_comp = ["classification_comp2", comp_fieldnames]

    csv_defs = [csv_comp]

    #csv_defs[0][0] = "classification_comp2.type_exp1"
    #test_config.run_in_test_environment(argv, "classification_type_exp1", csv_defs,      count_analysis_exp1)
    #csv_defs[0][0] = "classification_comp2.type_exp2"
    #test_config.run_in_test_environment(argv, "classification_type_exp2", csv_defs,   count_analysis_exp2)
    #csv_defs[0][0] = "classification_comp2.type_exp3"
    #test_config.run_in_test_environment(argv, "classification_type_exp3", csv_defs,      count_analysis_exp3)
    #csv_defs[0][0] = "classification_comp2.type_exp4"
    #test_config.run_in_test_environment(argv, "classification_type_exp4", csv_defs,      count_analysis_exp4)

    #csv_defs[0][0] = "classification_comp2.type_exp5"
    #test_config.run_in_test_environment(argv, "classification_type_exp5", csv_defs,      count_analysis_exp5)
    csv_defs[0][0] = "classification_comp2.type_exp6"
    test_config.run_in_test_environment(argv, "classification_type_exp6", csv_defs,   count_analysis_exp6)
    #csv_defs[0][0] = "classification_comp2.type_exp7"
    #test_config.run_in_test_environment(argv, "classification_type_exp7", csv_defs,      count_analysis_exp7)
    #csv_defs[0][0] = "classification_comp2.type_exp8"
    #test_config.run_in_test_environment(argv, "classification_type_exp8", csv_defs,      count_analysis_exp8)

#def main(argv):
#    test_dir = test_config.get_test_dir(argv)
#
#    with open(os.path.join(test_dir, "error_file_class_ext.O0"), "w") as e0:
#        with open(os.path.join(test_dir, "error_file_class_ext.O1"), "w") as e1:
#            with open(os.path.join(test_dir, "error_file_class_ext.O2"), "w") as e2:
#                with open(os.path.join(test_dir, "error_file_class_ext.O3"), "w") as e3:
#                    with open(os.path.join(test_dir, "error_file_class_ext.Os"), "w") as es:
#                        with open(os.path.join(test_dir, "classification_ext_cs.csv"), "w") as cs_csv_file:
#                            with open(os.path.join(test_dir, "classification_ext_ct.csv"), "w") as ct_csv_file:
#                                with open(os.path.join(test_dir, "compound_ext.csv"), "w") as compound_csv_file:
#
#                                    error_file = {}
#                                    error_file["O0"] = e0
#                                    error_file["O1"] = e1
#                                    error_file["O2"] = e2
#                                    error_file["O3"] = e3
#                                    error_file["Os"] = es
#
#                                    cs_fieldnames = ["target", "opt", "#cs", "problems", "+0", "+1", "+2", "+3", "+4", "+5", "+6", "non-void-ok", "non-void-problem"]
#                                    ct_fieldnames = ["target", "opt", "#ct", "problems", "-0", "-1", "-2", "-3", "-4", "-5", "-6", "void-ok", "void-problem"]
#                                    compound_fieldnames = ["opt", "target"]
#                                    compound_fieldnames += ["#cs", "cs args (perfect%)", "cs args (problem%)", "cs non-void (correct%)", "cs non-void (problem%)"]
#                                    compound_fieldnames += ["#ct", "ct args (perfect%)", "ct args (problem%)", "ct void (correct%)", "ct void (problem%)"]
#
#                                    cs_writer = csv.DictWriter(cs_csv_file, fieldnames=cs_fieldnames)
#                                    cs_writer.writeheader()
#                                    ct_writer = csv.DictWriter(ct_csv_file, fieldnames=ct_fieldnames)
#                                    ct_writer.writeheader()
#                                    compound_writer = csv.DictWriter(compound_csv_file, fieldnames=compound_fieldnames)
#                                    compound_writer.writeheader()
#
#                                    for test_target in test_config.configure_targets(argv):
#                                        result = test_target.verify_classification_ext_all()
#
#                                        for opt in result.keys():
#                                            (cs_dict, ct_dict, problem_string) = result[opt]
#                                            error_file[opt].write(problem_string)
#                                            cs_dict["opt"] = opt
#                                            ct_dict["opt"] = opt
#                                            cs_writer.writerow(cs_dict)
#                                            ct_writer.writerow(ct_dict)
#
#                                            compound_dict = {}
#                                            compound_dict["opt"] = opt
#                                            compound_dict["target"] = cs_dict["target"]
#                                            compound_dict["#cs"] = cs_dict["#cs"]
#                                            if 0 == int(cs_dict["#cs"]):
#                                                compound_dict["cs args (perfect%)"] = "-"
#                                                compound_dict["cs args (problem%)"] = "-"
#                                                compound_dict["cs non-void (correct%)"] = "-"
#                                                compound_dict["cs non-void (problem%)"] = "-"
#                                            else:
#                                                compound_dict["cs args (perfect%)"] = str(cs_dict["+0"]) + " (" + gen_pct_string(float(cs_dict["+0"]) / float(cs_dict["#cs"])) + "%)"
#                                                compound_dict["cs args (problem%)"] = str(cs_dict["problems"]) + " (" + gen_pct_string(float(cs_dict["problems"]) / float(cs_dict["#cs"])) + "%)"
#                                                compound_dict["cs non-void (correct%)"] = str(cs_dict["non-void-ok"]) + " (" + gen_pct_string(float(cs_dict["non-void-ok"]) / float(cs_dict["#cs"])) + "%)"
#                                                compound_dict["cs non-void (problem%)"] = str(cs_dict["non-void-problem"]) + " (" + gen_pct_string(float(cs_dict["non-void-problem"]) / float(cs_dict["#cs"])) + "%)"
#
#                                            compound_dict["#ct"] = ct_dict["#ct"]
#                                            if 0 == int(ct_dict["#ct"]):
#                                                compound_dict["ct args (perfect%)"] = "-"
#                                                compound_dict["ct args (problem%)"] = "-"
#                                                compound_dict["ct void (correct%)"] = "-"
#                                                compound_dict["ct void (problem%)"] = "-"
#                                            else:
#                                                compound_dict["ct args (perfect%)"] = str(ct_dict["-0"]) + " (" + gen_pct_string(float(ct_dict["-0"]) / float(ct_dict["#ct"])) + "%)"
#                                                compound_dict["ct args (problem%)"] = str(ct_dict["problems"]) + " (" + gen_pct_string(float(ct_dict["problems"]) / float(ct_dict["#ct"])) + "%)"
#                                                compound_dict["ct void (correct%)"] = str(ct_dict["void-ok"]) + " (" + gen_pct_string(float(ct_dict["void-ok"]) / float(ct_dict["#ct"])) + "%)"
#                                                compound_dict["ct void (problem%)"] = str(ct_dict["void-problem"]) + " (" + gen_pct_string(float(ct_dict["void-problem"]) / float(ct_dict["#ct"])) + "%)"
#
#                                            compound_writer.writerow(compound_dict)
#                                            

main(sys.argv[1:])
