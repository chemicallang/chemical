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
     * add given dependencies to the given module
     */
    static void add_dependencies(LabModule* mod, LabModule** dependencies, unsigned int dep_len);

    /**
     * it creates a flat vector containing pointers to lab modules, sorted
     *
     * It de-dupes, meaning avoids duplicates, it won't add two pointers
     * that are same, so dependencies that occur again and again, would
     * only be compiled once
     *
     * why sort ? Modules that should be compiled first are present first
     * The first module that should be compiled is at zero index, The last
     * module would be the given module, compiled at last
     *
     */
    static std::vector<LabModule*> flatten_dedupe_sorted(LabModule* mod);

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

};