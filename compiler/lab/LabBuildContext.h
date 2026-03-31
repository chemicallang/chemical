// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string.h"
#include "LabModule.h"
#include "LabJob.h"
#include "LabBuildCompilerOptions.h"
#include "compiler/cbi/model/CBIData.h"
#include "ModuleStorage.h"
#include <vector>
#include <unordered_map>
#include <optional>

std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<ModuleDependency>& modules);

class ImportPathHandler;

class CompilerBinder;

class LabBuildCompiler;

class ASTProcessor;

class ToCAstVisitor;

/**
 * stores all the data required
 */
class BasicBuildContext {
public:

    /**
     * the module storage
     */
    ModuleStorage& storage;

    /**
     * the compiler binder is used to provide the binding support
     */
    CompilerBinder& binder;

    /**
     * constructor
     */
    BasicBuildContext(
            ModuleStorage& storage,
            CompilerBinder& binder
    ) : storage(storage), binder(binder) {

    }

};

/**
 * A Lab build context is just a container
 * for all the modules user creates, build variables, any resources
 * It provides easier way to create modules that will be processed by
 * our compiler to create a flat set of files for each module that are
 * compiled one by one
 */
class LabBuildContext : public BasicBuildContext {
public:

    /**
     * build arguments given to the build lab
     */
    std::unordered_map<std::string, std::string> build_args;

    /**
     * if import paths are to be used with aliases in them, we need a path handler
     */
    ImportPathHandler& handler;

    /**
     * the compiler is available to build nested modules
     */
    LabBuildCompiler& compiler;

public:

    /**
     * constructor
     */
    LabBuildContext(
        LabBuildCompiler& compiler,
        ImportPathHandler& path_handler,
        ModuleStorage& storage,
        CompilerBinder& binder
    ) : handler(path_handler), compiler(compiler), BasicBuildContext(storage, binder) {

    }

    static void initialize_job(LabJob* job, LabBuildCompilerOptions* options, const std::string& target_triple);

    /**
     * initialize a new job from compiler options
     */
    inline static void initialize_job(LabJob* job, LabBuildCompilerOptions* options) {
        initialize_job(job, options, options->target_triple);
    }

    /**
     * declare alias for a path
     */
    static void declare_alias(std::unordered_map<std::string, std::string, StringHash, StringEqual>& aliases, std::string alias, std::string path);

    /**
     * add the given module as a directory module
     */
    LabModule* new_module(
            LabModuleType type,
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            PackageKind pkg_kind = PackageKind::Library
    );

    /**
     * add the given module as a directory module
     */
    inline LabModule* new_module(const chem::string_view& scope_name, const chem::string_view& module_name, PackageKind pkg_kind = PackageKind::Library) {
        return new_module(LabModuleType::Directory, scope_name, module_name, pkg_kind);
    }

    /**
     * puts the given job before the existing job
     */
    void put_job_before(LabJob* newJob, LabJob* existingJob);

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
    LabJob* translate_to_c(chem::string_view* name, chem::string_view* out_path);

    /**
     * adds an executable entry that'll be built
     */
    LabJob* build_exe(chem::string_view* name);

    /**
     * add a jit executable
     */
    LabJob* run_jit_exe(chem::string_view* name);

    /**
     * adds a library entry that'll be built
     */
    LabJob* build_dynamic_lib(chem::string_view* name);

    /**
     * adds a cbi entry that'll be built
     */
    LabJob* build_cbi(chem::string_view* name);

    /**
     * has this build argument
     */
    bool has_arg(const std::string& name);

    /**
     * consume this build argument
     */
    chem::string_view get_arg(const std::string& name);

    /**
     * remove this build argument
     */
    void remove_arg(const std::string& name);

};
