// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "LabBuildCompilerOptions.h"
#include "LabJob.h"
#include "ctpl.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "core/source/LocationManager.h"
#include "preprocess/ImportPathHandler.h"
#include "compiler/mangler/NameMangler.h"
#include "compiler/lab/ModuleStorage.h"
#include "compiler/processor/ModuleDependencyRecord.h"
#include "ast/base/TypeBuilder.h"
#include "compiler/frontend/AnnotationController.h"

class ASTAllocator;

class LabBuildContext;

class CTranslator;

struct GlobalContainer;

struct CmdOptions;

class ToCAstVisitor;

class TypeBuilder;

#ifdef COMPILER_BUILD
class Codegen;
#endif

/**
 * lab build compiler, doesn't relate to building a .lab file
 * it provides easy methods to do what can be done with a .lab file
 * The limited api is used to provide command line interface to our compiler applications
 */
class LabBuildCompiler {
public:

    /**
     * the annotation controller handles annotations
     */
    AnnotationController controller;

    /**
     * a single mangler to rule them all
     */
    NameMangler mangler;

    /**
     * the path handler is used to resolve paths, store aliases during compilation
     */
    ImportPathHandler path_handler;

    /**
     * allows us to index and storage module pointers that can be retrieved really
     * fast
     */
    ModuleStorage mod_storage;

    /**
     * compiler binder is used to bind compiler functions with user source code
     */
    CompilerBinder& binder;

    /**
     * the location manager is used to track locations inside all the files
     * processed by this compiler
     */
    LocationManager& loc_man;

    /**
     * lab build compiler options
     */
    LabBuildCompilerOptions* options;

    /**
     * creating a thread pool for all our jobs in the lab build
     * Initialize thread pool with the number of available hardware threads
     */
    ctpl::thread_pool pool;

    /**
     * when given, we check for any command line options that configure the code generator
     */
    CmdOptions* cmd = nullptr;

    /**
     * dependencies having build.lab have a function that return the LabModule*
     * instead of parsing the build.lab again and running it, we reuse this module
     * pointer
     */
    std::unordered_map<std::string, LabModule*> buildLabDependenciesCache;

    /**
     * the global container contains namespaces like std and compiler
     */
    GlobalContainer* container = nullptr;

    /**
     * mutex to protect access to the job object (remote_imports, dependencies, link_libs, etc.)
     */
    std::mutex job_mutex;

    /**
     * mutex to protect access to buildLabDependenciesCache
     */
    std::mutex buildLabDependenciesCacheMutex;

    /**
     * mutex to protect access to mod_storage
     */
    std::mutex mod_storage_mutex;

    /**
     * the current job being compiled
     */
    LabJob* current_job;

    /**
     * used to resolve remote imports conflicts manually, via command line
     */
    std::unordered_map<std::string, std::string> resolved_remote_imports;

    /**
     * the global allocator is used for things allocated for multiple jobs
     * like inside type builder to allocate types once
     */
    ASTAllocator global_allocator;

    /**
     * the type cache is initialized once for executing multiple jobs
     */
    TypeBuilder type_builder;

    /**
     * job level allocator
     */
    ASTAllocator* job_allocator;
    /**
     * module level allocator
     */
    ASTAllocator* mod_allocator;
    /**
     * file level allocator
     */
    ASTAllocator* file_allocator;

    /**
     * constructor
     */
    LabBuildCompiler(
        LocationManager& loc_man,
        CompilerBinder& binder,
        LabBuildCompilerOptions* options
    );

    /**
     * constructor
     */
    LabBuildCompiler(
            LocationManager& loc_man,
            CompilerBinder& binder,
            LabBuildCompilerOptions* options,
            int nThreads
    );

    /**
     * set the cmd options to allow checking for any command line options given to configure the code generation process
     */
    void set_cmd_options(CmdOptions* ptr) {
        cmd = ptr;
    }

    /**
     * sets the following allocators
     */
    inline void set_allocators(
            ASTAllocator* const jobAllocator,
            ASTAllocator* const modAllocator,
            ASTAllocator* const fileAllocator
    ) {
        job_allocator = jobAllocator;
        mod_allocator = modAllocator;
        file_allocator = fileAllocator;
    }

    /**
     * the given module is processed
     */
    int process_module_tcc(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string_view& build_dir
    );

    /**
     * the given module is processed
     */
    int process_module_tcc_bm(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string_view& build_dir
    );

#ifdef COMPILER_BUILD

    /**
     * the given module is processed
     */
    int process_module_gen(
            LabModule* mod,
            ASTProcessor& processor,
            Codegen& gen,
            CTranslator& cTranslator,
            const std::string_view& build_dir
    );

    /**
     * the given module is processed
     */
    int process_module_gen_bm(
            LabModule* mod,
            ASTProcessor& processor,
            Codegen& gen,
            CTranslator& cTranslator,
            const std::string_view& build_dir
    );

#endif

    /**
     * this is called by our own compiler to link tiny cc object files to
     * run them using jit
     */
    static int tcc_run_invocation(
        char* exe_path,
        std::vector<std::string_view>& obj_files,
        OutputMode mode,
        int argc,
        char** argv
    );

    /**
     * launches a compiler instance to link and run the program in memory (jit)
     * using tcc without emitting an executable
     */
    int launch_tcc_jit_exe(LabJob* job, std::vector<LabModule*>& dependencies);

    /**
     * cbi job
     */
    int link_cbi_job(LabJobCBI* job, std::vector<LabModule*>& dependencies);

    /**
     * use tcc to process the job
     */
    int process_job_tcc(LabJob* job);

#ifdef COMPILER_BUILD

    /**
     * process the job using gen
     */
    int process_job_gen(LabJob* job);

#endif

    /**
     * should use tcc for this job type
     */
    inline bool use_tcc(LabJobType type) {
        if(options->use_tcc) return true;
        switch(type) {
            case LabJobType::CBI:
            case LabJobType::ToCTranslation:
            case LabJobType::JITExecutable:
                return true;
            default:
                return false;
        }
    }

    /**
     * should use tcc for the job
     */
    inline bool use_tcc(LabJob* job) {
        return use_tcc(job->type);
    }

    /**
     * processes modules, generates code, or all modules required for linking but doesn't link
     * it also calls appropriate method process job tcc or process job gen
     */
    int process_modules(LabJob* job) {
#ifdef COMPILER_BUILD
        return use_tcc(job) ? (
            process_job_tcc(job)
        ) : (
            process_job_gen(job)
        );
#else
        return process_job_tcc(job);
#endif
    }

    /**
     * does executable job (generates executable)
     */
    int do_executable_job(LabJob* job);

    /**
     * does library job (generates shared object or dll)
     */
    int do_library_job(LabJob* job);

    /**
     * does job to translate c to chemical
     */
    int do_to_chemical_job(LabJob* job);

    /**
     * will perform the job returning result
     */
    int do_job(LabJob* job);

    /**
     * allocators will be set, memory will be allocated to call the given lambda
     */
    int do_allocating(void* data, int(*do_jobs)(LabBuildCompiler*, void*));

    /**
     * will perform the job, using appropriate allocators on stack
     */
    int do_job_allocating(LabJob* job);

    /**
     * we create a module for the following dependency, by importing and building
     * it's build.lab or chemical.mod file
     */
    LabModule* create_module_for_dependency(
            LabBuildContext& context,
            ModuleDependencyRecord& dependency,
            LabJob* job
    );

    /**
     * this translates a chemical.mod file into a build.lab file
     */
    static int translate_mod_file_to_lab(
        const chem::string_view& modFilePath,
        const chem::string_view& outputPath
    );

    /**
     * a chemical.mod file can be imported using this method into a LabModule*, the dependencies will include
     * all the modules this module depends on (the mod file states this, it recursively parses those modules)
     */
    LabModule* build_module_from_mod_file(
            LabBuildContext& context,
            const std::string_view& modFilePath,
            LabJob* job
    );

    /**
     * process remote imports for the job
     */
    int process_remote_imports(LabBuildContext& context, LabJob* job);

    /**
     * resolves a conflict between two remote imports
     */
    int resolve_remote_import_conflict(RemoteImport& existing, RemoteImport& current);

    /**
     * will build the lab file and return the callable tcc state
     * however using the given resources
     */
    TCCState* built_lab_file(
            LabBuildContext& context,
            const std::string_view& path,
            ASTProcessor& processor,
            ToCAstVisitor& c_visitor,
            bool mod_file_source,
            LabJob* job
    );

    /**
     * create a module out of mod file
     */
    LabModule* built_mod_file(LabBuildContext& context, const std::string_view& path, LabJob* job);

    /**
     * create a module out of a build.lab file
     */
    TCCState* built_lab_file(
            LabBuildContext& context,
            const std::string_view& path,
            bool mod_file_source
    );

    /**
     * will build the lab file
     */
    int build_lab_file(LabBuildContext& context, const std::string_view& path);

    /**
     * build the chemical.mod or build.lab file at path, with the given job
     * assuming the build.lab and chemical.mod will return a module pointer
     */
    int build_module_build_file(
            LabBuildContext& context,
            const std::string_view& path,
            LabJob* final_job,
            bool mod_file_source
    );

    /**
     * a module level build.lab file returns a module pointer
     * processing for building it in a job is different
     */
    inline int build_module_lab_file(LabBuildContext& context, const std::string_view& path, LabJob* final_job) {
        return build_module_build_file(context, path, final_job, false);
    }

    /**
     * build the mod file given at path, into an executable at outputPath
     */
    inline int build_mod_file(LabBuildContext& context, const std::string_view& path, LabJob* final_job) {
        return build_module_build_file(context, path, final_job, true);
    }

    /**
     * run invocation point, dispatches to local, remote or transformer run
     */
    static int run_invocation(
        const std::string& exe_path,
        const std::string& target,
        const std::vector<std::string_view>& args,
        OutputMode mode,
        CmdOptions* cmd_options
    );

    /**
     * the destructor
     */
    ~LabBuildCompiler();

private:
    /**
     * runs a local project (.lab or .mod)
     * the target is the file (.lab or .mod)
     */
    int run_local_project(
            const std::string& target,
            chem::string outputPath,
            const std::vector<std::string_view>& args,
            LabBuildContext& context
    );

    /**
     * runs a remote module
     */
    int run_remote_module(const std::string& target, const std::vector<std::string_view>& args, LabBuildContext& context);

    /**
     * runs a transformer
     */
    int run_transformer(const std::string& transformer, const std::string& target, const std::vector<std::string_view>& args, LabBuildContext& context);

    /**
     * gets the centralized cache directory for chemical commands
     */
    static std::string get_commands_cache_dir();

};
