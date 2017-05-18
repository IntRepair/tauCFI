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

    count_data2 = []
    for (_, sites, targets) in count_real_baseline:
        count_data2 += [targets for x in range(sites)]

    type_data = []
    for (_, sites, targets) in type_baseline:
        type_data += [targets for x in range(sites)]

    type_data2 = []
    for (_, sites, targets) in type_real_baseline:
        type_data2 += [targets for x in range(sites)]

    maximum_hi = max([max(count_data), max(count_data2), max(type_data), max(type_data2)])
    maximum_len = max([len(count_data), len(count_data2), len(type_data), len(type_data2)])

    sorted_data = np.sort(count_data) / float(maximum_hi)
    xvals = np.arange(len(sorted_data))/ float(len(sorted_data))
    plt.plot(xvals, sorted_data, '--', label="count*")


    sorted_data = np.sort(count_data2) / float(maximum_hi)
    xvals = np.arange(len(sorted_data)) / float(len(sorted_data))
    plt.plot(xvals, sorted_data, '-.', label="count")



    sorted_data = np.sort(type_data) / float(maximum_hi)
    xvals = np.arange(len(sorted_data)) / float(len(sorted_data))
    plt.plot(xvals, sorted_data, "-", label="type*")


    sorted_data = np.sort(type_data2) / float(maximum_hi)
    xvals = np.arange(len(sorted_data)) / float(len(sorted_data))
    plt.plot(xvals, sorted_data, ':', label="type")

    plt.xlabel('Ratio of indirect Callsites')
    plt.ylabel('Ratio of Calltargets')
    plt.title(target + " " + opt)

    axes = plt.gca()
    axes.set_xlim([0,1.0])
    axes.set_ylim([0,1.0])
    axes.xaxis.set_ticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0])
    axes.yaxis.set_ticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0])

    plt.legend(loc='lower right')
    plt.tight_layout()
    plt.grid(True)
    plt.savefig(os.path.join(directory, target + "." + opt + ".svg"))
    plt.gcf().clear()
    #plt.show()

def main(argv):
    test_dir = test_config.get_test_dir(argv)

    for test_target in test_config.configure_targets(argv):
        result = test_target.generate_cdf_compare_all()

        for opt in result.keys():
            ((count_baseline, count_real_baseline, type_baseline, type_real_baseline), problem_string) = result[opt]
            plot_baselines(count_baseline, count_real_baseline, type_baseline, type_real_baseline, test_target.name(), test_dir, opt)

main(sys.argv[1:])
