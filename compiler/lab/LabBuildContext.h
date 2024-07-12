// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "LabJob.h"
#include "integration/ide/model/FlatIGFile.h"
#include <vector>
#include <unordered_map>

std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules);

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
    std::vector<LabJob> executables;
    // build arguments given to the build lab
    std::unordered_map<std::string, std::string> build_args;

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
     * add given dependencies to the given module
     */
    static void add_paths(std::vector<chem::string>& into, chem::string** paths, unsigned int path_len);

    /**
     * adds the given module with type
     */
    LabModule* add_with_type(
            LabModuleType type,
            chem::string name,
            chem::string** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * adds the given module with type file
     */
    LabModule* file_module(
            chem::string* name,
            chem::string* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Files, name->copy(), &path, 1, dependencies, dep_len);
    }

    /**
     * adds the given module with type multiple files
     */
    LabModule* files_module(
            chem::string* name,
            chem::string** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Files, name->copy(), paths, path_len, dependencies, dep_len);
    }

    /**
     * adds the given module with type multiple files
     */
    LabModule* c_file_module(
            chem::string* name,
            chem::string* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::CFile, name->copy(), &path, 1, dependencies, dep_len);
    }

    /**
     * adds an executable entry that'll be built
     */
    LabJob* build_exe(
            chem::string* name,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * adds a library entry that'll be built
     */
    LabJob* build_dynamic_lib(
            chem::string* name,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * provides build path into given string
     */
    void build_path(chem::string* str) {
        str->append(build_dir);
    }

    /**
     * has this build argument
     */
    bool has_arg(chem::string* name);

    /**
     * consume this build argument
     */
    void get_arg(chem::string* str, chem::string* name);

    /**
     * remove this build argument
     */
    void remove_arg(chem::string* name);

};