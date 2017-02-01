import csv
import os
import sys

import test_config

import matplotlib.pyplot as plt
import numpy as np

def plot_baselines(count_baseline, count_real_baseline, type_baseline, type_real_baseline, target, directory, opt):
    count_data = []
    for (_, sites, targets) in count_baseline:
        count_data += [targets for x in range(sites)]

    sorted_data = np.sort(count_data) / float(max(count_data))
    yvals = np.arange(len(sorted_data)) / float(len(sorted_data))

    plt.plot(yvals, sorted_data, '--', label="count*")

    count_data2 = []
    for (_, sites, targets) in count_real_baseline:
        count_data2 += [targets for x in range(sites)]

    sorted_data = np.sort(count_data2) / float(max(count_data))
    yvals = np.arange(len(sorted_data)) / float(len(sorted_data))

    plt.plot(yvals, sorted_data, '-.', label="count")


    type_data = []
    for (_, sites, targets) in type_baseline:
        type_data += [targets for x in range(sites)]

    sorted_data = np.sort(type_data) / float(max(count_data))

    yvals = np.arange(len(sorted_data)) / float(len(sorted_data))
    plt.plot(yvals, sorted_data, label="type*")


    type_data = []
    for (_, sites, targets) in type_real_baseline:
        type_data += [targets for x in range(sites)]

    sorted_data = np.sort(type_data) / float(max(count_data))

    yvals = np.arange(len(sorted_data)) / float(len(sorted_data))
    plt.plot(yvals, sorted_data, ':', label="type")

    plt.xlabel('Ratio of indirect Callsites')
    plt.ylabel('Ratio of Calltargets')
    plt.title(target + " " + opt)

    axes = plt.gca()
    axes.set_xlim([0,1.0])
    axes.set_ylim([0,1.0])

    plt.legend(loc='lower right')
    plt.tight_layout()
    plt.savefig(os.path.join(directory, target + "." + opt + ".svg"))
    plt.gcf().clear()
#    plt.show()

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    for test_target in test_config.configure_targets(argv):
        result = test_target.generate_cdf_compare_all()

        for opt in result.keys():
            ((count_baseline, count_real_baseline, type_baseline, type_real_baseline), problem_string) = result[opt]
            plot_baselines(count_baseline, count_real_baseline, type_baseline, type_real_baseline, test_target.name(), test_dir, opt)

main(sys.argv[1:])
