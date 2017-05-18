import sys

import test_config
import harness.utilities as utils

def count_analysis(argv, error_files, csv_writers):
    cs_writers = csv_writers["classification_cs"]
    ct_writers = csv_writers["classification_ct"]
    compound_writers = csv_writers["classification_comp"]

    for test_target in test_config.configure_targets(argv):
        result = test_target.verify_classification_all()

        for opt in result.keys():
            (cs_dict, ct_dict, problem_string) = result[opt]
            error_files[opt].write(problem_string)
            cs_dict["opt"] = opt
            ct_dict["opt"] = opt
            cs_writers[opt].writerow(cs_dict)
            ct_writers[opt].writerow(ct_dict)

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
                compound_dict["cs args perfect"] = str(cs_dict["+0"])
                compound_dict["cs args perfect pct"] = utils.gen_pct_string(float(cs_dict["+0"]) / float(cs_dict["cs"]))
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
                compound_dict["ct args perfect"] = str(ct_dict["-0"])
                compound_dict["ct args perfect pct"] = utils.gen_pct_string(float(ct_dict["-0"]) / float(ct_dict["ct"]))
                compound_dict["ct args problem"] = str(ct_dict["problems"])
                compound_dict["ct args problem pct"] = utils.gen_pct_string(float(ct_dict["problems"]) / float(ct_dict["ct"]))
                compound_dict["ct void correct"] = str(ct_dict["void-ok"])
                compound_dict["ct void correct pct"] = utils.gen_pct_string(float(ct_dict["void-ok"]) / float(ct_dict["ct"]))
                compound_dict["ct void problem"] = str(ct_dict["void-problem"])
                compound_dict["ct void problem pct"] = utils.gen_pct_string(float(ct_dict["void-problem"]) / float(ct_dict["ct"]))

            compound_writers[opt].writerow(compound_dict)  


def main(argv):
    ct_fieldnames = ["target", "opt", "ct", "problems", "-0", "-1", "-2", "-3", "-4", "-5", "-6", "void-ok", "void-problem"]
    csv_cs = ("classification_ct", ct_fieldnames)

    cs_fieldnames = ["target", "opt", "cs", "problems", "+0", "+1", "+2", "+3", "+4", "+5", "+6", "non-void-ok", "non-void-problem"]
    csv_ct = ("classification_cs", cs_fieldnames)

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

    csv_comp = ("classification_comp", comp_fieldnames)

    csv_defs = [csv_cs, csv_ct, csv_comp]

    test_config.run_in_test_environment(argv, "classification", csv_defs, count_analysis)

main(sys.argv[1:])
