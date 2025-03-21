// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "LabJob.h"
#include "integration/cbi/model/FlatIGFile.h"
#include "LabBuildCompilerOptions.h"
#include "parser/model/CBIData.h"
#include "ModuleStorage.h"
#include <vector>
#include <unordered_map>

class ImportPathHandler;

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

    /**
     * the place where modules exist
     */
    ModuleStorage storage;
    // the build directory that will be used for file generation
    std::string build_dir;
    // all the executables created during the build process
    std::vector<std::unique_ptr<LabJob>> executables;
    // build arguments given to the build lab
    std::unordered_map<std::string, std::string> build_args;
    // the lambda that will be called on exist
    void(*on_finished)(void*) = nullptr;
    // the data pointer that'll be passed to on_finished at end
    void* on_finished_data = nullptr;
    // if import paths are to be used with aliases in them, we need a path handler
    ImportPathHandler& handler;
    // the compiler options sent by the user
    LabBuildCompilerOptions* options = nullptr;

private:

    /**
     * get a module with file path, of type
     */
    LabModule* create_of_type(LabModuleType type, chem::string_view* path, unsigned number);

public:

    /**
     * constructor
     */
    LabBuildContext(
        LabBuildCompilerOptions* options,
        ImportPathHandler& path_handler,
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
     * add given dependencies to the given module
     */
    static void add_paths(std::vector<chem::string>& into, chem::string_view** paths, unsigned int path_len);

    /**
     * declare alias for a path
     */
    static void declare_alias(std::unordered_map<std::string, std::string>& aliases, std::string alias, std::string path);

    /**
     * declare alias for a path for user into given job
     * this method is used by the cbi, if the path has any aliases we replace those
     * by getting them from the job
     * returns true if declared
     */
    bool declare_user_alias(LabJob* job, std::string alias, std::string path);

    /**
     * module with it's dependent modules will be declared with it's name
     * to the absolute path alias, if the module is a directory module
     */
    static void put_path_aliases(LabJob* job, LabModule* module);

    /**
     * all the modules of this job will be declared with path aliases
     * including the indirect dependent modules
     */
    static void init_path_aliases(LabJob* job);

    /**
     * adds the given module with type
     */
    LabModule* add_with_type(
            LabModuleType type,
            chem::string_view* name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    );

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
     * adds the given module with type multiple files
     */
    LabModule* files_module(
            chem::string_view* name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * when path's list contains only chemical files
     */
    LabModule* chemical_files_module(
            chem::string_view* name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Files, name, paths, path_len, dependencies, dep_len);
    }

    /**
     * add the given module as a directory module
     */
    LabModule* chemical_dir_module(
            chem::string_view* name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Directory, name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as a directory module
     */
    LabModule* chemical_dir_module(
        chem::string* name,
        chem::string* path,
        LabModule** dependencies,
        unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Directory, name->copy(), &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as a c translation unit
     */
    LabModule* c_file_module(
            chem::string_view* name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::CFile, name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as a c translation unit
     */
    LabModule* cpp_file_module(
            chem::string_view* name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::CPPFile, name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as an obj file, that'll be linked with final executable
     */
    LabModule* obj_file_module(
            chem::string_view* name,
            chem::string_view* path
    ) {
        return add_with_type(LabModuleType::ObjFile, name, &path, 1, nullptr, 0);
    }

    /**
     * translate a c file to chemical
     */
    LabJob* translate_to_chemical(
        LabModule* module,
        chem::string_view* out_path
    );

    /**
     * will set the build directory for the given job
     */
    void set_build_dir(
        LabJob* job
    );

    /**
     * translate a chemical file to c
     */
    LabJob* translate_to_c(
            chem::string_view* name,
            LabModule** dependencies,
            unsigned int dep_len,
            chem::string_view* out_path
    );

    /**
     * adds an executable entry that'll be built
     */
    LabJob* build_exe(
            chem::string_view* name,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * adds a library entry that'll be built
     */
    LabJob* build_dynamic_lib(
            chem::string_view* name,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * adds a cbi entry that'll be built
     */
    LabJob* build_cbi(
            chem::string_view* name,
            LabModule** dependencies,
            unsigned int dep_len,
            LabModule* entry
    );

    /**
     * has this build argument
     */
    bool has_arg(const std::string& name);

    /**
     * consume this build argument
     */
    void get_arg(chem::string& str, const std::string& name);

    /**
     * remove this build argument
     */
    void remove_arg(const std::string& name);

};