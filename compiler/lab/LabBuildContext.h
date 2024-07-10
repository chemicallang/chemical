// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "LabExecutable.h"
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

    // the build directory that will be used for file generation
    std::string build_dir;
    // all the modules created during the build process
    std::vector<LabModule> modules;
    // all the executables created during the build process
    std::vector<LabExecutable> executables;

    /**
     * constructor
     */
    explicit LabBuildContext(
        std::string lab_file,
        std::string build_dir = ""
    );

    /**
     * add given dependencies to the given module
     */
    static void add_dependencies(std::vector<LabModule*>& into, LabModule** dependencies, unsigned int dep_len);

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
     * same as above, only it operates on multiple modules, it de-dupes the dependent modules
     * of the given list of modules and also sorts them
     */
    static std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules);

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

    /**
     * adds an executable entry that'll be built
     */
    LabExecutable* build_exe(
            chem::string* name,
            LabModule** dependencies,
            unsigned int dep_len
    );

};