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

std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules);

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
    * all the executables created during the build process
    */
    std::vector<std::unique_ptr<LabJob>> executables;

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


// splitmix64 mixer (good distribution, tiny and fast)
static inline uint64_t splitmix64(uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

// Key: single pointer + two string_views (non-owning)
struct JobAndModuleRelKey {
    LabJob* ptr = nullptr;
    chem::string_view scope_name;
    chem::string_view mod_name;
    bool operator==(JobAndModuleRelKey const& o) const noexcept {
        return ptr == o.ptr && scope_name == o.scope_name && mod_name == o.mod_name;
    }
};

// Hash functor mixing pointer and two string_view hashes
struct JobAndModuleRelHash {
    size_t operator()(JobAndModuleRelKey const& k) const noexcept {
        // turn pointer into an integer
        const auto p = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(k.ptr));
        // hash the string_views with std::hash
        const auto h1 = static_cast<uint64_t>(std::hash<chem::string_view>{}(k.scope_name));
        const auto h2 = static_cast<uint64_t>(std::hash<chem::string_view>{}(k.mod_name));

        // combine them into a single 64-bit value using a non-commutative combine,
        // then run splitmix64 to get well-distributed bits.
        uint64_t combined = p;
        combined = combined + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2);
        combined ^= (h1 + 0x9e3779b97f4a7c15ULL);
        combined = combined + 0x9e3779b97f4a7c15ULL + (h2 << 6) + (h2 >> 2);
        combined ^= (h2 + 0x9e3779b97f4a7c15ULL);

        uint64_t mixed = splitmix64(combined);

        if constexpr (sizeof(size_t) < 8) {
            // fold to 32-bit if necessary
            return static_cast<size_t>(mixed ^ (mixed >> 32));
        } else {
            return static_cast<size_t>(mixed);
        }
    }
};

using JobAndModuleRelMap = std::unordered_map<JobAndModuleRelKey, LabModule*, JobAndModuleRelHash>;

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
     * you can use it to check whether a module has been built for a specific job
     */
    JobAndModuleRelMap existance_cache;

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
    ) : handler(path_handler), compiler(compiler), BasicBuildContext(storage, binder) {

    }

    /**
     * initialize a new job from compiler options
     */
    static void initialize_job(LabJob* job, LabBuildCompilerOptions* options);

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
    static void add_paths(std::vector<chem::string>& into, chem::string_view** paths, unsigned int path_len);

    /**
     * declare alias for a path
     */
    static void declare_alias(std::unordered_map<std::string, std::string, StringHash, StringEqual>& aliases, std::string alias, std::string path);

    /**
     * does the module exists in cache (associated with job)
     */
    LabModule* get_cached(LabJob* job, const chem::string_view& scope_name, const chem::string_view& mod_name) {
        auto found = existance_cache.find(JobAndModuleRelKey{ .ptr = job, .scope_name = scope_name, .mod_name = mod_name });
        return found != existance_cache.end() ? found->second : nullptr;
    }

    /**
     * set a module exists in cache (associated with job)
     */
    void set_exists_in_cache(LabJob* job, LabModule* module) {
        existance_cache[JobAndModuleRelKey{ .ptr = job, .scope_name = module->scope_name.to_chem_view(), .mod_name = module->name.to_chem_view() }] = module;
    }

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
     * add the given module as a directory module
     */
    LabModule* new_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name
    ) {
        return add_with_type(LabModuleType::Directory, scope_name, module_name, nullptr, 0, nullptr, 0);
    }

    /**
     * add the given module as a directory module
     */
    inline LabModule* new_module(
            const chem::string_view& scope_name,
            const chem::string_view& module_name,
            LabModule** dependencies,
            unsigned int dep_len
    ) {
        return add_with_type(LabModuleType::Directory, scope_name, module_name, nullptr, 0, dependencies, dep_len);
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
    void get_arg(chem::string& str, const std::string& name);

    /**
     * remove this build argument
     */
    void remove_arg(const std::string& name);

};