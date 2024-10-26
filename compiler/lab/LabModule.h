// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "LabModuleType.h"
#include "integration/cbi/model/FlatIGFile.h"
#include "std/chem_string.h"

struct LabModuleHeader {
    chem::string path;
    bool is_known_system_header;
};

struct LabModule {

    // type of the module
    LabModuleType type;
    // name of the module
    chem::string name;
    // the translated c file (if any) will be written to this path
    chem::string out_c_path;
    // bitcode file path for this module
    chem::string bitcode_path;
    // object file path for this module
    chem::string object_path;
    // if not empty, module's llvm ir is written to at this path
    chem::string llvm_ir_path;
    // if not empty, module's assembly is written to at this path
    chem::string asm_path;
    // includes are chemical files included in the modules
    // these files are imported in the module by the compiler
    std::vector<chem::string> includes;
    // these headers are imported before any other files are processed
    std::vector<chem::string> headers;
    // this path point to a c (.c, .h) file, a chemical file, a directory or a build.lab
    // depends on the type of module
    std::vector<chem::string> paths;
    // dependencies are the pointers to modules that this module depends on
    // these modules will be compiled first
    std::vector<LabModule*> dependencies;

};