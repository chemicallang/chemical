// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "LabModuleType.h"
#include "integration/ide/model/FlatIGFile.h"
#include "std/chem_string.h"

struct LabModule {

    // type of the module
    LabModuleType type;
    // name of the module
    chem::string name;
    // this path point to a c (.c, .h) file, a chemical file, a directory or a build.lab
    // depends on the type of module
    chem::string path;
    // dependencies are the pointers to modules that this module depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;

};