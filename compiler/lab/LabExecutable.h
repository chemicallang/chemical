// Copyright (c) Qinetik 2024.

#include "std/chem_string.h"

struct LabExecutable {
    // name of the executable, this name shouldn't contain .exe at the end
    chem::string name;
    // absolute path to the executable which is determined by the compiler
    chem::string abs_path;
    // absolute path to build dir path
    chem::string build_dir;
    // dependencies are the pointers to modules that this module depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;
};