import csv
import os
import sys

import harness.utilities as utils

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

    summary_fieldnames = ["target", "opt", "policy"]
    summary_fieldnames += ["safe cts"]
    summary_fieldnames += ["conserv cts"]
    summary_fieldnames += ["our cts"]

    summary_result = {}


    with open(os.path.join(test_dir, "policy_baselines_safe.csv"), "w") as policy_csv_file:
        policy_writer = csv.DictWriter(policy_csv_file, fieldnames=policy_fieldnames)
        policy_writer.writeheader()

        for test_target in test_config.configure_targets(argv):
            result = test_target.generate_policy_baselines_safe_all()

            for opt in result.keys():
                (policy_dicts, problem_string) = result[opt]
                for policy_dict in policy_dicts:
                    policy_dict["opt"] = opt
                    policy_writer.writerow(policy_dict)
                    summary_dict = {}
                    summary_dict["target"] = policy_dict["target"]
                    summary_dict["opt"] = policy_dict["opt"]
                    summary_dict["policy"] = policy_dict["policy"]
                    summary_dict["safe cts"] = policy_dict["summary cts"]
                    utils.add_values_to_key(summary_result, opt, summary_dict)


    with open(os.path.join(test_dir, "policy_baselines_conserv.csv"), "w") as policy_csv_file:
        policy_writer = csv.DictWriter(policy_csv_file, fieldnames=policy_fieldnames)
        policy_writer.writeheader()

        for test_target in test_config.configure_targets(argv):
            result = test_target.generate_policy_baselines_conserv_all()

            for opt in result.keys():
                (policy_dicts, problem_string) = result[opt]
                for policy_dict in policy_dicts:
                    policy_dict["opt"] = opt
                    policy_writer.writerow(policy_dict)
                    summary_dict = [item for item in summary_result[opt] if item["target"] == policy_dict["target"] and item["policy"] == policy_dict["policy"]]
                    summary_dict[0]["conserv cts"] = policy_dict["summary cts"]

    with open(os.path.join(test_dir, "policy_baselines_ours.csv"), "w") as policy_csv_file:
        policy_writer = csv.DictWriter(policy_csv_file, fieldnames=policy_fieldnames)
        policy_writer.writeheader()

        for test_target in test_config.configure_targets(argv):
            result = test_target.generate_policy_baselines_ours_all()

            for opt in result.keys():
                (policy_dicts, problem_string) = result[opt]
                for policy_dict in policy_dicts:
                    policy_dict["opt"] = opt
                    policy_writer.writerow(policy_dict)
                    summary_dict = [item for item in summary_result[opt] if item["target"] == policy_dict["target"] and item["policy"] == policy_dict["policy"]]
                    summary_dict[0]["our cts"] = policy_dict["summary cts"]

    with open(os.path.join(test_dir, "policy_baselines_summary.csv"), "w") as summary_csv_file:
        summary_writer = csv.DictWriter(summary_csv_file, fieldnames=summary_fieldnames)
        summary_writer.writeheader()

        for opt in summary_result.keys():
            summary_dicts = summary_result[opt]
            for summary_dict in summary_dicts:
                summary_writer.writerow(summary_dict)

main(sys.argv[1:])
