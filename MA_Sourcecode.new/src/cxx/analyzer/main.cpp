#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <assert.h>

#include <cstdlib>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_binaryEdit.h>
#include <BPatch_image.h>
#include <BPatch_object.h>

#include "analysis.h"
#include "ast/ast.h"
#include "ca_decoder.h"
#include "utils.h"
#include "verification.h"

/* Usage : handler $binary [--force-rewrite]
 */

void printHelp() { fprintf(stderr, "Usage : handler $binary [--help]\n"); }

int main(int argc, char **argv) {
  signal(SIGSEGV, handler); // install our handler

  if (argc < 2) {
    printHelp();
    return -1;
  }

  const std::string path = argv[1];

  TypeShield::analysis::config_t analysis_config;

  for (int i = 2; i < argc; ++i) {
    const std::string arg(argv[i]);

    if (arg == std::string("--help")) {
      printHelp();
      return 0;
    } else if (arg == std::string("--calltargets")) {
      analysis_config.flags = TypeShield::analysis::flags_t(
          analysis_config.flags | TypeShield::analysis::CALLTARGET_ANALYSIS);
    } else if (arg == std::string("--callsites")) {
      analysis_config.flags = TypeShield::analysis::flags_t(
          analysis_config.flags | TypeShield::analysis::CALLSITE_ANALYSIS);
    } else if (arg == std::string("--types")) {
      analysis_config.type = TypeShield::analysis::TYPE_ANALYSIS;
    } else if (arg == std::string("--calltarget_config") && i + 1 < argc) {
      analysis_config.ct_liveness_config = std::atoi(argv[++i]);
    } else if (arg == std::string("--callsite_config") && i + 1 < argc) {
      analysis_config.cs_reaching_config = std::atoi(argv[++i]);
    } else {
      printHelp();
      return -1;
    }
  }

  BPatch bpatch;
  BPatch_binaryEdit *handle = bpatch.openBinary(path.c_str());
  BPatch_image *image = handle->getImage();

  CADecoder decoder;

  auto const is_system = [](BPatch_object *object) {
    std::vector<BPatch_module *> modules;
    object->modules(modules);
    if (modules.size() != 1)
      return false;
    return modules[0]->isSystemLib();
  };

  auto const is_excluded_library = [](BPatch_object *object) {
    const std::array<std::string, 16> excluded_libraries{
        "libdyninstAPI_RT.so", "libpthread.so", "libz.so",    "libdl.so",
        "libcrypt.so",         "libnsl.so",     "libm.so",    "libstdc++.so",
        "libgcc_s.so",         "libc.so",       "libpcre.so", "libcrypto.so",
        "libcap.so",           "libpthread-",   "libdl-",     "libc-"};

    auto const library_name = object->name();
    for (auto const &exclude : excluded_libraries) {
      if (exclude.length() <= library_name.length())
        if (std::equal(exclude.begin(), exclude.end(), library_name.begin()))
          return true;
    }
    return false;
  };

  std::vector<BPatch_object *> objects;
  image->getObjects(objects);
  for (auto object : objects) {
    LOG_TRACE(LOG_FILTER_GENERAL, "Processing object %s",
              object->name().c_str());

    if (!(is_system(object) || is_excluded_library(object))) {
      LOG_INFO(LOG_FILTER_GENERAL, "Processing object %s",
               object->name().c_str());

      auto const ast = ast::ast::create(object, &decoder);

      auto taken_address_count =
          util::count_if(ast.get_functions(), [](ast::function const &fn) {
            return fn.get_is_at();
          });

      auto callsite_count = 0;
      for (auto const &function : ast.get_functions()) {
        callsite_count += util::count_if(function.get_basic_blocks(),
                                         [](ast::basic_block const &bb) {
                                           return bb.get_is_indirect_callsite();
                                         });
      }

      LOG_INFO(LOG_FILTER_GENERAL,
               "Found %lu taken_addresses, %lu calltargets and %d callsites",
               taken_address_count, ast.get_functions().size(), callsite_count);

      auto result = TypeShield::analysis::run(ast, analysis_config, &decoder);

      verification::output(ast, result);
    }
  }

  LOG_INFO(LOG_FILTER_GENERAL, "Finished Dyninst setup, returning to target");

  return 0;
}
