import csv
import os
import sys

import test_config
import harness.utilities as utils

def policy_analysis(argv, error_files, csv_writers):
    policy_compare_writers = csv_writers["policy_compare_at"]

    geomean_dict = {}

    geomean_dict["opt"] = "O2"
    geomean_dict["target"] = "geomean"

    for test_target in test_config.configure_targets(argv):
        result = test_target.generate_policy_compare_our_at_all()

        for opt in result.keys():
            (policy_dicts, problem_string) = result[opt]

            target_dict = {}

            for policy_dict in policy_dicts:
                target_policy_dict = {}
                if policy_dict["target"] in target_dict.keys():
                    target_policy_dict = target_dict[policy_dict["target"]]
                else:
                    target_policy_dict["target"] = policy_dict["target"]
                    target_policy_dict["opt"] = opt

                if policy_dict["policy"] == "AT":
                    target_policy_dict["at"] = policy_dict["summary cts avg"]
                elif policy_dict["policy"] == "TYPE*":
                    target_policy_dict["type* avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["type* sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["type* median"] = policy_dict["summary cts median"]
                elif policy_dict["policy"] == "TYPE_SAFE":
                    target_policy_dict["type safe avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["type safe sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["type safe median"] = policy_dict["summary cts median"]
                elif policy_dict["policy"] == "TYPE_PREC":
                    target_policy_dict["type prec avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["type prec sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["type prec median"] = policy_dict["summary cts median"]
                elif policy_dict["policy"] == "COUNT*":
                    target_policy_dict["count* avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["count* sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["count* median"] = policy_dict["summary cts median"]
                elif policy_dict["policy"] == "COUNT_SAFE":
                    target_policy_dict["count safe avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["count safe sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["count safe median"] = policy_dict["summary cts median"]
                elif policy_dict["policy"] == "COUNT_PREC":
                    target_policy_dict["count prec avg"] = policy_dict["summary cts avg"]
                    target_policy_dict["count prec sig"] = policy_dict["summary cts sig"]
                    target_policy_dict["count prec median"] = policy_dict["summary cts median"]

                target_dict[policy_dict["target"]] = target_policy_dict

            for key in target_dict.keys():
                policy_compare_writers[opt].writerow(target_dict[key])

            for key in target_policy_dict.keys():
                if key not in ["opt", "target"]:
                    utils.copy_data(geomean_dict, target_policy_dict, opt, key)

    for key in geomean_dict.keys():
        if key not in ["opt", "target"]:
            geomean_dict[key] = utils.geomean(geomean_dict[key])

    policy_compare_writers["O2"].writerow(geomean_dict)

def main(argv):
    comp_fieldnames = ["opt", "target"]

    comp_fieldnames += ["at"]
    comp_fieldnames += ["count safe avg", "count safe sig", "count safe median"]
    comp_fieldnames += ["count prec avg", "count prec sig", "count prec median"]
    comp_fieldnames += ["count* avg", "count* sig", "count* median"]
    comp_fieldnames += ["type safe avg", "type safe sig", "type safe median"]
    comp_fieldnames += ["type prec avg", "type prec sig", "type prec median"]
    comp_fieldnames += ["type* avg", "type* sig", "type* median"]

    csv_comp = ("policy_compare_at", comp_fieldnames)

    csv_defs = [csv_comp]

    test_config.run_in_test_environment(argv, "policy_compare_at", csv_defs, policy_analysis)

main(sys.argv[1:])
