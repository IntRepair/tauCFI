import sys

import test_config
import harness.utilities as utils

def matching_analysis(argv, error_files, csv_writers):
    matching_writer = csv_writers["matching"]

    geomean_dict = {}

    geomean_dict["opt"] = "O2"
    geomean_dict["target"] = "geomean"

    for test_target in test_config.configure_targets(argv):
        result = test_target.verify_matching_all()
        for opt in result.keys():
            (csv_data, problem_string) = result[opt]
            error_files[opt].write(problem_string)
            csv_data["opt"] = opt
            matching_writer[opt].writerow(csv_data)

            for key in csv_data.keys():
                if key not in ["opt", "target"]:
                    utils.copy_data(geomean_dict, csv_data, opt, key)

    for key in geomean_dict.keys():
        if key not in ["opt", "target"]:
            geomean_dict[key] = utils.geomean(geomean_dict[key])

    matching_writer["O2"].writerow(geomean_dict)

def main(argv):
    fieldnames = ["target"]
    fieldnames += ["opt"]

    subfieldnames = ["clang", "clang pct", "padyn", "padyn pct"]

    fieldnames += ["fn"] + ["fn not in " + s for s in subfieldnames]
    fieldnames += ["at"] + ["at not in " + s for s in subfieldnames]
    fieldnames += ["cs"] + ["cs discarded " + s for s in subfieldnames]

    csv_matching = ("matching", fieldnames)

    csv_defs = [csv_matching]

    test_config.run_in_test_environment(argv, "matching", csv_defs, matching_analysis)

main(sys.argv[1:])
