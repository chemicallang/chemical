// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "integration/ide/model/FlatIGFile.h"
#include <vector>

/**
 * A Lab build context is just a container
 * for all the modules user creates, build variables, any resources
 * It provides easier way to create modules that will be processed by
 * our compiler to create a flat set of files for each module that are
 * compiled one by one
 */
class LabBuildContext {
public:

    // all the modules created during the build process
    std::vector<LabModule> modules;
    // the pointer to root module, that must be set before any build begins
    LabModule* root_module = nullptr;

    /**
     * adds the given module with type
     */
    LabModule* add_with_type(
            LabModuleType type,
            chem::string* name,
            chem::string* path,
            LabModule** dependencies,
            unsigned int dep_len
    );

    static void add_dependencies(LabModule* mod, LabModule** dependencies, unsigned int dep_len);

};