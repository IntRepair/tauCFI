import sys
import test_config

def main(argv):
    for test_target in test_config.configure_targets(argv):
        test_target.analyze_all()

main(sys.argv[1:])
