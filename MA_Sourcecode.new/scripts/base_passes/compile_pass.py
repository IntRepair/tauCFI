import os

from test_pass import Pass

class CompilePass(Pass):
    def __init__(self, config, args):
        Pass.__init__(self, "compile", config, args)

    def build_environment(self, target):
        llvm_dir = os.path.join(self.config["type_shield.dir"], self.config["type_shield.llvm.build_dir"], "bin")

        command_string = ""
        command_string += "AR=\"" + os.path.join(llvm_dir, "llvm-ar") + "\" "
        command_string += "LD=\"" + os.path.join(llvm_dir, "lld") + "\" "
        command_string += "CC=\"" + os.path.join(llvm_dir, "clang") + "\" "
        command_string += "CXX=\"" + os.path.join(llvm_dir, "clang++") + "\" "
        command_string += "CFLAGS=\"" + target.cc_options + "\" "
        command_string += "CXXFLAGS=\"" + target.cc_options + "\" "
        command_string += "LDFLAGS=\"" + target.ld_options + "\" "

        return command_string

    def build_standard_config(self, target):
        command_string = "cd " + target.sourcedir + ";"
        command_string += "sudo make clean; "
        command_string += self.build_environment(target)
        command_string += "./configure --prefix=\"" + target.prefix + "\""
        return command_string

    def build_standard_make(self, target):
        command_string = "cd " + target.sourcedir + "; "
        #command_string += self.build_environment(target)
        command_string += "make 2> " + os.path.join(target.prefix, "ground_truth." + target.binary_name)
        return command_string

    def build_standard_install(self, target):
        command_string = "cd " + target.sourcedir + "; "
        # is this still important ?
        # command_string += "sudo make install 2>> " + os.path.join(target.prefix, "ground_truth." + target.binary_name)  + "; "
        binary_source = os.path.join(target.sourcedir_binary, target.binary_name)
        binary_target = os.path.join(target.prefix, target.binary_name)
        command_string += "cp " + binary_source + " " + binary_target
        return command_string

    def _compile_exim(self, target):
        pass

    def _compile_nginx(self, target):
	llvm_dir = os.path.join(self.config["type_shield.dir"], self.config["type_shield.llvm.build_dir"], "bin")

        command_string = "cd " + target.sourcedir + "; "
        command_string += "sudo make clean; "
        command_string += "./configure --prefix=\"" + target.prefix + "\" "
        command_string += "--with-cc=\"" + os.path.join(llvm_dir, "clang") + "\" "
        command_string += "--with-cc-opt=\"" + target.cc_options + "\" "
        command_string += "--with-ld-opt=\"" + target.ld_options + "\" "
        command_string += "; "

        command_string += self.build_standard_make(target)
        command_string += "; "

        command_string += self.build_standard_install(target)
        command_string += "; "

        print command_string
        os.system(command_string)

    def _compile_postgresql(self, target):
        command_string = self.build_standard_config(target)
        # TODO why do we need this ?
        command_string += " --disable-thread-safety"
        command_string += "; "

        command_string += self.build_standard_make(target)
        command_string += "; "

        command_string += self.build_standard_install(target)
        command_string += "; "

        print command_string
        os.system(command_string)

    # TODO simplify this or can we even replace this ?
    def _sub_configure_vsftpd(self, target):
        llvm_dir = os.path.join(self.config["type_shield.dir"], self.config["type_shield.llvm.build_dir"], "bin")
        makefile_path = os.path.join(target.sourcedir, "Makefile")
        patched_makefile_path = makefile_path + ".patched"
        patched_makefile = file(patched_makefile_path, 'w')
        with file(makefile_path, 'r') as makefile:
            for line in makefile:
                if line.startswith("CC"):
                    tokens = line.split("=")
                    line = tokens[0] + "=" + os.path.join(llvm_dir, "clang") + "\n"
                elif line.startswith("LINK"):
                    tokens = line.split("=")
                    line = tokens[0] + "=" + target.ld_options + "\n"

                elif line.startswith("LIBS"):
                    tokens = line.split("=")
                    sub_tokens = tokens[1].split(" ")

                    line = tokens[0] + "="

                    if "-ldl" not in sub_tokens:
                        line += " -ldl"

                    if "-lcap" not in sub_tokens:
                        line += " -lcap"

                    if "-lcrypt" not in sub_tokens:
                        line += " -lcrypt"

                    line += tokens[1]

                elif line.startswith("CFLAGS"):
                    tokens = line.split("=")
                    sub_tokens = tokens[1].strip().split(" ")
                    line = tokens[0] + "="
                    for sub_token in sub_tokens:
                        sub_token = sub_token.strip()
                        if sub_token in ["-O0", "-O1", "-O2", "-O3", "-Os"]:
                            sub_token = target.cc_options
                        if sub_token in ["-flto", "-g"]:
                            sub_token = ""
                        if sub_token != "":
                            line += " " + sub_token
                    line += "\n"
                patched_makefile.write(line)

        os.rename(patched_makefile_path, makefile_path)

    def _compile_vsftpd(self, target):
        self._sub_configure_vsftpd(target)

        command_string = "cd " + os.path.join(target.sourcedir, target.sourcedir_binary) + "; "
        command_string += "make clean; "

        command_string += self.build_standard_make(target)
        command_string += "; "

        command_string += self.build_standard_install(target)
        command_string += "; "

        print command_string
        os.system(command_string)

    def _compile_config_make(self, target):
        command_string = self.build_standard_config(target)
        command_string += "; "

        command_string += self.build_standard_make(target)
        command_string += "; "

        command_string += self.build_standard_install(target)
        command_string += "; "

        print command_string
        os.system(command_string)

    def _compile_default(self, target):
        pass

    def _run_on_target(self, target):
        if not os.path.exists(target.prefix):
            os.makedirs(target.prefix)

        if not self.args["force"]:
            binary = os.path.join(target.prefix, target.binary_name)
            if os.path.exists(binary) and os.path.isfile(binary):
                return

        if target.name == "exim":
            self._compile_exim(target)
        elif target.name == "nginx":
            self._compile_nginx(target)
        elif target.name == "postgresql":
            self._compile_postgresql(target)
        elif target.name == "vsftpd":
            self._compile_vsftpd(target)
        elif target.method == "config+make":
            self._compile_config_make(target)
        else:
            self._compile_default(target)

    def _run_on_targets(self, targets):
        for target in targets:
            self._run_on_target(target)
