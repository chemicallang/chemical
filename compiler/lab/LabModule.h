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
    // bitcode file path for this module
    chem::string bitcode_path;
    // object file path for this module
    chem::string object_path;
    // if not empty, module's llvm ir is written to at this path
    chem::string llvm_ir_path;
    // if not empty, module's assembly is written to at this path
    chem::string asm_path;
    // this path point to a c (.c, .h) file, a chemical file, a directory or a build.lab
    // depends on the type of module
    std::vector<chem::string> paths;
    // dependencies are the pointers to modules that this module depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;

};