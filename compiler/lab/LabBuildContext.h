// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "LabJob.h"
#include "compiler/cbi/model/FlatIGFile.h"
#include "LabBuildCompilerOptions.h"
#include "compiler/cbi/model/CBIData.h"
#include "ModuleStorage.h"
#include <vector>
#include <unordered_map>
#include <optional>

std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules);

class ImportPathHandler;

class CompilerBinder;

class LabBuildCompiler;

class ASTProcessor;

class ToCAstVisitor;

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
     * all the executables created during the build process
     */
    std::vector<std::unique_ptr<LabJob>> executables;

    /**
     * build arguments given to the build lab
     */
    std::unordered_map<std::string, std::string> build_args;

    /**
     * the lambda that will be called on exist
     */
    void(*on_finished)(void*) = nullptr;

    /**
     * the data pointer that'll be passed to on_finished at end
     */
    void* on_finished_data = nullptr;

    /**
     * if import paths are to be used with aliases in them, we need a path handler
     */
    ImportPathHandler& handler;

    /**
     * the module storage
     */
    ModuleStorage& storage;

    /**
     * the compiler binder is used to provide the binding support
     */
    CompilerBinder& binder;

    /**
     * the compiler is available to build nested modules
     */
    LabBuildCompiler& compiler;

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
        LabBuildCompiler& compiler,
        ImportPathHandler& path_handler,
        ModuleStorage& storage,
        CompilerBinder& binder,
        std::string lab_file
    ) : handler(path_handler), compiler(compiler), storage(storage), binder(binder) {

    }

    /**
     * add given dependencies to the given module
     */
    static void add_dependencies(std::vector<LabModule*>& into, LabModule** dependencies, unsigned int dep_len);

    /**
     * add given dependencies to the given module, this should be used when putting dependencies
     * into a module, because this takes care of informing about dependents as well
     */
    static void add_dependencies(LabModule* module, LabModule** dependencies, unsigned int dep_len);

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
     * the path to a directory is given, in which we can find a build.lab or a
     * chemical.mod file, which is compiled to generate a Module*
     * if error occurs we set it in error_msg and null is returned
     */
    LabModule* module_from_directory(const chem::string_view& path, const chem::string_view& scope_name, const chem::string_view& mod_name, chem::string& error_msg);

    /**
     * adds the given module with type
     */
    LabModule* add_with_type(
            LabModuleType type,
            const chem::string_view& scope_name,
            const chem::string_view& name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * adds the given module with type multiple files
     */
    LabModule* files_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    );

    /**
     * when path's list contains only chemical files
     */
    LabModule* chemical_files_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view** paths,
            unsigned int path_len,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Files, scope_name, module_name, paths, path_len, dependencies, dep_len);
    }

    /**
     * add the given module as a directory module
     */
    LabModule* chemical_dir_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Directory, scope_name, module_name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as a c translation unit
     */
    LabModule* c_file_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::CFile, scope_name, module_name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as a c translation unit
     */
    LabModule* cpp_file_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view* path,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::CPPFile, scope_name, module_name, &path, 1, dependencies, dep_len);
    }

    /**
     * add the given module as an obj file, that'll be linked with final executable
     */
    LabModule* obj_file_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            chem::string_view* path
    ) {
        return add_with_type(LabModuleType::ObjFile, scope_name, module_name, &path, 1, nullptr, 0);
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
            unsigned int dep_len
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