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
     * a directory with no build.lab file, is considered a directory module
     * files inside are sorted so that independent files that don't depend on other files
     * are compiled first
     */
    LabModule* dir_module(
        chem::string* name,
        chem::string* path,
        LabModule** dependencies,
        unsigned int dep_len
    );

};