import getopt
import test_target
import os

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

    CC_Options = "-g" #"-flto"#-pipe -W -Wall -Wpointer-arith -Wno-unused"#" -fno-optimize-sibling-calls"
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
    #LD_Options += "-B " + BINUTILS + "/gold -Wl,-plugin " + LLVMBUILD + "/lib/LLVMgold.so"

    test_targets = []
    test_generator = test_target.TargetGenerator(ANALYSISSCRIPT, TESTDIR, CCompiler, CXXCompiler, CC_Options, LD_Options)

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
