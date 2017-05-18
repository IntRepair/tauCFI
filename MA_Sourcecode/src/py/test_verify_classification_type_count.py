import sys

import test_config
import harness.utilities as utils

def count_analysis(argv, error_files, csv_writers):
    cs_comp_writers = csv_writers["cs_comp"]
    ct_comp_writers = csv_writers["ct_comp"]

    for test_target in test_config.configure_targets(argv):
        result_count = test_target.verify_classification_all()
        result_type = test_target.verify_classification_type_all()

        for opt in result_count.keys():
            (cs_count_dict, ct_count_dict, problem_string) = result_count[opt]
            (cs_type_dict, ct_type_dict, problem_string) = result_type[opt]
            error_files[opt].write(problem_string)

            cs_compound_dict = {}
            cs_compound_dict["opt"] = opt
            cs_compound_dict["target"] = cs_count_dict["target"]
            cs_compound_dict["cs"] = cs_count_dict["cs"]
            if 0 == int(cs_count_dict["cs"]):
                cs_compound_dict["count cs args perfect"] = "0"
                cs_compound_dict["count cs args perfect pct"] = "0"
                cs_compound_dict["count cs args problem"] = "0"
                cs_compound_dict["count cs args problem pct"] = "0"
                cs_compound_dict["count cs non-void correct"] = "0"
                cs_compound_dict["count cs non-void correct pct"] = "0"
                cs_compound_dict["count cs non-void problem"] = "0"
                cs_compound_dict["count cs non-void problem pct"] = "0"
            else:
                cs_compound_dict["count cs args perfect"] = str(cs_count_dict["+0"])
                cs_compound_dict["count cs args perfect pct"] = utils.gen_pct_string(float(cs_count_dict["+0"]) / float(cs_count_dict["cs"]))
                cs_compound_dict["count cs args problem"] = str(cs_count_dict["problems"])
                cs_compound_dict["count cs args problem pct"] = utils.gen_pct_string(float(cs_count_dict["problems"]) / float(cs_count_dict["cs"]))
                cs_compound_dict["count cs non-void correct"] = str(cs_count_dict["non-void-ok"])
                cs_compound_dict["count cs non-void correct pct"] = utils.gen_pct_string(float(cs_count_dict["non-void-ok"]) / float(cs_count_dict["cs"]))
                cs_compound_dict["count cs non-void problem"] = str(cs_count_dict["non-void-problem"])
                cs_compound_dict["count cs non-void problem pct"] = utils.gen_pct_string(float(cs_count_dict["non-void-problem"]) / float(cs_count_dict["cs"]))

            if 0 == int(cs_type_dict["cs"]):
                cs_compound_dict["type cs args perfect"] = "0"
                cs_compound_dict["type cs args perfect pct"] = "0"
                cs_compound_dict["type cs args problem"] = "0"
                cs_compound_dict["type cs args problem pct"] = "0"
                cs_compound_dict["type cs non-void correct"] = "0"
                cs_compound_dict["type cs non-void correct pct"] = "0"
                cs_compound_dict["type cs non-void problem"] = "0"
                cs_compound_dict["type cs non-void problem pct"] = "0"
            else:
                cs_compound_dict["type cs args perfect"] = str(cs_type_dict["perfect"])
                cs_compound_dict["type cs args perfect pct"] = utils.gen_pct_string(float(cs_type_dict["perfect"]) / float(cs_type_dict["cs"]))
                cs_compound_dict["type cs args problem"] = str(cs_type_dict["problems"])
                cs_compound_dict["type cs args problem pct"] = utils.gen_pct_string(float(cs_type_dict["problems"]) / float(cs_type_dict["cs"]))
                cs_compound_dict["type cs non-void correct"] = str(cs_type_dict["non-void-ok"])
                cs_compound_dict["type cs non-void correct pct"] = utils.gen_pct_string(float(cs_type_dict["non-void-ok"]) / float(cs_type_dict["cs"]))
                cs_compound_dict["type cs non-void problem"] = str(cs_type_dict["non-void-problem"])
                cs_compound_dict["type cs non-void problem pct"] = utils.gen_pct_string(float(cs_type_dict["non-void-problem"]) / float(cs_type_dict["cs"]))

            cs_comp_writers[opt].writerow(cs_compound_dict)


            ct_compound_dict = {}
            ct_compound_dict["opt"] = opt
            ct_compound_dict["target"] = ct_count_dict["target"]
            ct_compound_dict["ct"] = ct_count_dict["ct"]
            if 0 == int(ct_count_dict["ct"]):
                ct_compound_dict["count ct args perfect"] = "0"
                ct_compound_dict["count ct args perfect pct"] = "0"
                ct_compound_dict["count ct args problem"] = "0"
                ct_compound_dict["count ct args problem pct"] = "0"
                ct_compound_dict["count ct void correct"] = "0"
                ct_compound_dict["count ct void correct pct"] = "0"
                ct_compound_dict["count ct void problem"] = "0"
                ct_compound_dict["count ct void problem pct"] = "0"
            else:
                ct_compound_dict["count ct args perfect"] = str(ct_count_dict["-0"])
                ct_compound_dict["count ct args perfect pct"] = utils.gen_pct_string(float(ct_count_dict["-0"]) / float(ct_count_dict["ct"]))
                ct_compound_dict["count ct args problem"] = str(ct_count_dict["problems"])
                ct_compound_dict["count ct args problem pct"] = utils.gen_pct_string(float(ct_count_dict["problems"]) / float(ct_count_dict["ct"]))
                ct_compound_dict["count ct void correct"] = str(ct_count_dict["void-ok"])
                ct_compound_dict["count ct void correct pct"] = utils.gen_pct_string(float(ct_count_dict["void-ok"]) / float(ct_count_dict["ct"]))
                ct_compound_dict["count ct void problem"] = str(ct_count_dict["void-problem"])
                ct_compound_dict["count ct void problem pct"] = utils.gen_pct_string(float(ct_count_dict["void-problem"]) / float(ct_count_dict["ct"]))

            if 0 == int(ct_type_dict["ct"]):
                ct_compound_dict["type ct args perfect"] = "0"
                ct_compound_dict["type ct args perfect pct"] = "0"
                ct_compound_dict["type ct args problem"] = "0"
                ct_compound_dict["type ct args problem pct"] = "0"
                ct_compound_dict["type ct void correct"] = "0"
                ct_compound_dict["type ct void correct pct"] = "0"
                ct_compound_dict["type ct void problem"] = "0"
                ct_compound_dict["type ct void problem pct"] = "0"
            else:
                ct_compound_dict["type ct args perfect"] = str(ct_type_dict["perfect"])
                ct_compound_dict["type ct args perfect pct"] = utils.gen_pct_string(float(ct_type_dict["perfect"]) / float(ct_type_dict["ct"]))
                ct_compound_dict["type ct args problem"] = str(ct_type_dict["problems"])
                ct_compound_dict["type ct args problem pct"] = utils.gen_pct_string(float(ct_type_dict["problems"]) / float(ct_type_dict["ct"]))
                ct_compound_dict["type ct void correct"] = str(ct_type_dict["void-ok"])
                ct_compound_dict["type ct void correct pct"] = utils.gen_pct_string(float(ct_type_dict["void-ok"]) / float(ct_type_dict["ct"]))
                ct_compound_dict["type ct void problem"] = str(ct_type_dict["void-problem"])
                ct_compound_dict["type ct void problem pct"] = utils.gen_pct_string(float(ct_type_dict["void-problem"]) / float(ct_type_dict["ct"]))
 
            ct_comp_writers[opt].writerow(ct_compound_dict)

def main(argv):
    cs_comp_fieldnames = ["opt", "target"]
    cs_comp_fieldnames += ["cs"]
    cs_comp_fieldnames += ["count cs args perfect"]
    cs_comp_fieldnames += ["count cs args perfect pct"]
    cs_comp_fieldnames += ["count cs args problem"]
    cs_comp_fieldnames += ["count cs args problem pct"]
    cs_comp_fieldnames += ["count cs non-void correct"]
    cs_comp_fieldnames += ["count cs non-void correct pct"]
    cs_comp_fieldnames += ["count cs non-void problem"]
    cs_comp_fieldnames += ["count cs non-void problem pct"]
    cs_comp_fieldnames += ["type cs args perfect"]
    cs_comp_fieldnames += ["type cs args perfect pct"]
    cs_comp_fieldnames += ["type cs args problem"]
    cs_comp_fieldnames += ["type cs args problem pct"]
    cs_comp_fieldnames += ["type cs non-void correct"]
    cs_comp_fieldnames += ["type cs non-void correct pct"]
    cs_comp_fieldnames += ["type cs non-void problem"]
    cs_comp_fieldnames += ["type cs non-void problem pct"]
    cs_csv_comp = ("cs_comp", cs_comp_fieldnames)

    ct_comp_fieldnames = ["opt", "target"]
    ct_comp_fieldnames += ["ct"]
    ct_comp_fieldnames += ["count ct args perfect"]
    ct_comp_fieldnames += ["count ct args perfect pct"]
    ct_comp_fieldnames += ["count ct args problem"]
    ct_comp_fieldnames += ["count ct args problem pct"]
    ct_comp_fieldnames += ["count ct void correct"]
    ct_comp_fieldnames += ["count ct void correct pct"]
    ct_comp_fieldnames += ["count ct void problem"]
    ct_comp_fieldnames += ["count ct void problem pct"]
    ct_comp_fieldnames += ["type ct args perfect"]
    ct_comp_fieldnames += ["type ct args perfect pct"]
    ct_comp_fieldnames += ["type ct args problem"]
    ct_comp_fieldnames += ["type ct args problem pct"]
    ct_comp_fieldnames += ["type ct void correct"]
    ct_comp_fieldnames += ["type ct void correct pct"]
    ct_comp_fieldnames += ["type ct void problem"]
    ct_comp_fieldnames += ["type ct void problem pct"]
    ct_csv_comp = ("ct_comp", ct_comp_fieldnames)

    csv_defs = [cs_csv_comp, ct_csv_comp]

    test_config.run_in_test_environment(argv, "classification_type_count", csv_defs, count_analysis)

main(sys.argv[1:])
