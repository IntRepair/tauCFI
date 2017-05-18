import csv
import os
import sys

import test_config
import harness.utilities as utils

geomean_dicts = {}

def type_analysis(argv, error_files, csv_writers, tag):
    compound_writers = csv_writers["classification_comp." + tag]

    geomean_dict = {}

    geomean_dict["opt"] = "O2"
    geomean_dict["target"] = "geomean"

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
                compound_dict["cs args perfect"] = "0"
                compound_dict["cs args perfect pct"] = "0"
                compound_dict["cs args problem"] = "0"
                compound_dict["cs args problem pct"] = "0"
                compound_dict["cs non-void correct"] = "0"
                compound_dict["cs non-void correct pct"] = "0"
                compound_dict["cs non-void problem"] = "0"
                compound_dict["cs non-void problem pct"] = "0"
            else:
                compound_dict["cs args perfect"] = str(cs_dict["perfect"])
                compound_dict["cs args perfect pct"] = utils.gen_pct_string(float(cs_dict["perfect"]) / float(cs_dict["cs"]))
                compound_dict["cs args problem"] = str(cs_dict["problems"])
                compound_dict["cs args problem pct"] = utils.gen_pct_string(float(cs_dict["problems"]) / float(cs_dict["cs"]))
                compound_dict["cs non-void correct"] = str(cs_dict["non-void-ok"])
                compound_dict["cs non-void correct pct"] = utils.gen_pct_string(float(cs_dict["non-void-ok"]) / float(cs_dict["cs"]))
                compound_dict["cs non-void problem"] = str(cs_dict["non-void-problem"])
                compound_dict["cs non-void problem pct"] = utils.gen_pct_string(float(cs_dict["non-void-problem"]) / float(cs_dict["cs"]))

            compound_dict["ct"] = ct_dict["ct"]
            if 0 == int(ct_dict["ct"]):
                compound_dict["ct args perfect"] = "0"
                compound_dict["ct args perfect pct"] = "0"
                compound_dict["ct args problem"] = "0"
                compound_dict["ct args problem pct"] = "0"
                compound_dict["ct void correct"] = "0"
                compound_dict["ct void correct pct"] = "0"
                compound_dict["ct void problem"] = "0"
                compound_dict["ct void problem pct"] = "0"
            else:
                compound_dict["ct args perfect"] = str(ct_dict["perfect"])
                compound_dict["ct args perfect pct"] = utils.gen_pct_string(float(ct_dict["perfect"]) / float(ct_dict["ct"]))
                compound_dict["ct args problem"] = str(ct_dict["problems"])
                compound_dict["ct args problem pct"] = utils.gen_pct_string(float(ct_dict["problems"]) / float(ct_dict["ct"]))
                compound_dict["ct void correct"] = str(ct_dict["void-ok"])
                compound_dict["ct void correct pct"] = utils.gen_pct_string(float(ct_dict["void-ok"]) / float(ct_dict["ct"]))
                compound_dict["ct void problem"] = str(ct_dict["void-problem"])
                compound_dict["ct void problem pct"] = utils.gen_pct_string(float(ct_dict["void-problem"]) / float(ct_dict["ct"]))

            compound_writers[opt].writerow(compound_dict)  

            for key in compound_dict.keys():
                if key not in ["opt", "target"]:
                    utils.copy_data(geomean_dict, compound_dict, opt, key)

    for key in geomean_dict.keys():
        if key not in ["opt", "target"]:
            geomean_dict[key] = utils.geomean(geomean_dict[key])

    geomean_dicts[tag] = geomean_dict

    compound_writers["O2"].writerow(geomean_dict)

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

    csv_comp = ["classification_comp", comp_fieldnames]

    csv_defs = [csv_comp]

    key_count = 24

    for index in range(key_count):
        tag = str(index) + "type"
        csv_defs[0][0] = "classification_comp." + tag
        test_config.run_in_test_environment(argv, "classification_" + tag, csv_defs, (lambda argv, error_files, csv_writers: type_analysis(argv, error_files, csv_writers, tag)))


    for index in range(key_count):
        tag = str(index) + "type" 
        entry = geomean_dicts[tag]

        print tag,
        print "CS perfect(pct) ", entry["cs args perfect pct"],
        print "CS problem(count) ", entry["cs args problem"],
        print "CT perfect(pct) ", entry["ct args perfect pct"],
        print "CT problem(count) ", entry["ct args problem"]

main(sys.argv[1:])
