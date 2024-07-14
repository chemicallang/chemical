// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "std/chem_string.h"
#include "LabJobType.h"

struct LabModule;

struct LabJob {
    // the type of job
    LabJobType type;
    // name of the job / executable / lib
    chem::string name;
    // absolute path to the job's output (exe or lib)
    chem::string abs_path;
    // absolute path to build dir for this job
    chem::string build_dir;
    // these are linkable object or bitcode files required by the module
    std::vector<chem::string> linkables;
    // dependencies are the pointers to modules that this module depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;
};