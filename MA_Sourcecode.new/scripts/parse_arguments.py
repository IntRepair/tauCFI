import getopt

def parse_arguments(argv, flag_options=[], value_options=[], multi_value_options=[]):
    arguments = {}

    # Initialize flag options to false
    for flag in flag_options:
        arguments[flag] = False

    option_strings = [x for x in flag_options]
    option_strings += [x + "=" for x in value_options]
    option_strings += [x + "=" for x in multi_value_options]
    opts, args = getopt.getopt(argv, "", option_strings)

    for opt, arg in opts:
        option = opt[2:]
        if option in flag_options:
            print "using flag: " + option
            arguments[option] = True
        elif option in value_options:
            print "using " + option + ": " + arg
            arguments[option] = arg
        elif option in multi_value_options:
            mult_arg = arg.split(",")
            print "using " + option + ": " + str(mult_arg)
            arguments[option] = mult_arg

    return arguments

