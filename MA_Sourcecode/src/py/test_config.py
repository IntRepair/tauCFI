import getopt
import test_target
import os
import csv

def get_test_dir(argv):
    option_strings = ["binutils-dir=", "llvm-build-dir=", "source-dir=", "test-dir=", "analysis-script="]
    opts, args = getopt.getopt(argv, "", option_strings)

    TESTDIR = ""

    for opt, arg in opts:
        if opt == "--test-dir":
            TESTDIR=arg
            print "using test-dir: " + arg

    return TESTDIR

def configure_targets(argv):
    option_strings = ["binutils-dir=", "llvm-build-dir=", "source-dir=", "test-dir=", "analysis-script="]
    opts, args = getopt.getopt(argv, "", option_strings)

    BINUTILS = ""
    LLVMBUILD = ""
    SOURCEDIR = ""
    TESTDIR = ""
    ANALYSISSCRIPT = ""

    CC_Options = "" #"-flto"#-pipe -W -Wall -Wpointer-arith -Wno-unused"#" -fno-optimize-sibling-calls"
    LD_Options = "" #"-flto -O0 -use-gold-plugin"#-Wl,-disable-opt"#-fno-optimize-sibling-calls"

    for opt, arg in opts:
        if opt == "--binutils-dir":
            print "using binutils: " + arg
            BINUTILS=arg
        elif opt == "--llvm-build-dir":
            print "using llvm-build-dir: " + arg
            LLVMBUILD=arg
        elif opt == "--source-dir":
            SOURCEDIR=arg
            print "using source-dir: " + arg
        elif opt == "--test-dir":
            TESTDIR=arg
            print "using test-dir: " + arg
        elif opt == "--analysis-script":
            ANALYSISSCRIPT=arg
            print "using analysis-script: " + arg

    CCompiler = os.path.join(LLVMBUILD, "bin", "clang")
    CXXCompiler = os.path.join(LLVMBUILD, "bin", "clang++")
    Linker = os.path.join(LLVMBUILD, "bin", "lld")
    #LD_Options += "-B " + BINUTILS + "/gold -Wl,-plugin " + LLVMBUILD + "/lib/LLVMgold.so"

    test_targets = []
    test_generator = test_target.TargetGenerator(ANALYSISSCRIPT, TESTDIR, CCompiler, CXXCompiler, Linker, CC_Options, LD_Options)

    TEST_CONFIG = os.path.join(SOURCEDIR, "tests.config")
    with file(TEST_CONFIG) as test_config_file:
        for line in test_config_file:
            line = line.split("#")[0] # ignore comments
            line = line.strip()
            if len(line) > 0:
                tokens = line.split(":")
                name = tokens[0]
                source = os.path.join(SOURCEDIR, tokens[1])
                method = tokens[2]
                binary_path = tokens[3]
                sourcedir_binary = tokens[4]
                test_targets += [ test_generator.generate_target(name, source, method, binary_path, sourcedir_binary) ]

    #TESTDIR += "/nginx"
    return test_targets #[test_target.Target("nginx", ANALYSISSCRIPT, SOURCEDIR, TESTDIR, CCompiler, CC_Options, LD_Options)]


def __run_in_test_environment(argv, name, csv_defs, fn, error_files, csv_writers):
    csv_def = csv_defs[0]
    csv_defs = csv_defs[1:]
    csv_name = csv_def[0]
    csv_fields = csv_def[1]
    test_dir = get_test_dir(argv)
    with open(os.path.join(test_dir, csv_name + ".O2.csv"), "w") as csv_file2:
        __csv_writers = {}
        writer = csv.DictWriter(csv_file2, fieldnames=csv_fields)
        writer.writeheader()
        __csv_writers["O2"] = writer
        csv_writers[csv_name] = __csv_writers

        if len(csv_defs) > 0:
            __run_in_test_environment(argv, name, csv_defs, fn, error_files, csv_writers)
        else:
            fn(argv, error_files, csv_writers)

    #with open(os.path.join(test_dir, csv_name + ".O0.csv"), "w") as csv_file0:
    #    with open(os.path.join(test_dir, csv_name + ".O1.csv"), "w") as csv_file1:
    #        with open(os.path.join(test_dir, csv_name + ".O2.csv"), "w") as csv_file2:
    #            with open(os.path.join(test_dir, csv_name + ".O3.csv"), "w") as csv_file3:
    #                with open(os.path.join(test_dir, csv_name + ".Os.csv"), "w") as csv_files:
    #                                        
    #                    __csv_writers = {}
    #                    writer = csv.DictWriter(csv_file0, fieldnames=csv_fields)
    #                    writer.writeheader()
    #                    __csv_writers["O0"] = writer
    #                    writer = csv.DictWriter(csv_file1, fieldnames=csv_fields)
    #                    writer.writeheader()
    #                    __csv_writers["O1"] = writer
    #                    writer = csv.DictWriter(csv_file2, fieldnames=csv_fields)
    #                    writer.writeheader()
    #                    __csv_writers["O2"] = writer
    #                    writer = csv.DictWriter(csv_file3, fieldnames=csv_fields)
    #                    writer.writeheader()
    #                    __csv_writers["O3"] = writer
    #                    writer = csv.DictWriter(csv_files, fieldnames=csv_fields)
    #                    writer.writeheader()
    #                    __csv_writers["Os"] = writer
    #                    csv_writers[csv_name] = __csv_writers
#
    #                    if len(csv_defs) > 0:
    #                        __run_in_test_environment(argv, name, csv_defs, fn, error_files, csv_writers)
    #                    else:
    #                        fn(argv, error_files, csv_writers)


def run_in_test_environment(argv, name, csv_defs, fn):
    test_dir = get_test_dir(argv)
    with open(os.path.join(test_dir, "error_file" + name + ".O2"), "w") as e2:
        error_files = {}
        error_files["O2"] = e2
        __run_in_test_environment(argv, name, csv_defs, fn, error_files, {})

    #with open(os.path.join(test_dir, "error_file" + name + ".O0"), "w") as e0:
    #    with open(os.path.join(test_dir, "error_file" + name + ".O1"), "w") as e1:
    #        with open(os.path.join(test_dir, "error_file" + name + ".O2"), "w") as e2:
    #            with open(os.path.join(test_dir, "error_file" + name + ".O3"), "w") as e3:
    #                with open(os.path.join(test_dir, "error_file" + name + ".Os"), "w") as es:
    #                    error_files = {}
    #                    error_files["O0"] = e0
    #                    error_files["O1"] = e1
    #                    error_files["O2"] = e2
    #                    error_files["O3"] = e3
    #                    error_files["Os"] = es
    #                    __run_in_test_environment(argv, name, csv_defs, fn, error_files, {})