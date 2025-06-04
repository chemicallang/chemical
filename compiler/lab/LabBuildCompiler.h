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

class ASTAllocator;

class LabBuildContext;

class CTranslator;

struct GlobalContainer;

class CmdOptions;

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
    LocationManager loc_man;

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
     * a pointer to current job
     */
    LabJob* current_job;

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
    explicit LabBuildCompiler(
        CompilerBinder& binder,
        LabBuildCompilerOptions* options
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
        const std::string& mod_timestamp_file,
        const std::string& out_c_file,
        bool do_compile,
        std::stringstream& output_ptr
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
            const std::string& mod_timestamp_file
    );

#endif

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
        return options->use_tcc || type == LabJobType::ToCTranslation || type == LabJobType::CBI;
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
     * link process modules result
     */
    int link(std::vector<chem::string>& result, const std::string& path, bool use_tcc);

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
            ModuleDependencyRecord& dependency
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
            const std::string_view& modFilePath
    );

    /**
     * will build the lab file and return the callable tcc state
     * however using the given resources
     */
    TCCState* built_lab_file(
            LabBuildContext& context,
            ModuleDependencyRecord& dependency,
            const std::string_view& path,
            ASTProcessor& processor,
            ToCAstVisitor& c_visitor,
            std::stringstream& output_ptr,
            bool mod_file_source
    );

    /**
     * create a module out of mod file
     */
    LabModule* built_mod_file(LabBuildContext& context, const std::string_view& path);

    /**
     * create a module out of a build.lab file
     */
    TCCState* built_lab_file(
            LabBuildContext& context,
            ModuleDependencyRecord& dependency,
            const std::string_view& path,
            bool mod_file_source
    );

    /**
     * will build the lab file
     */
    int build_lab_file(LabBuildContext& context, const std::string_view& path);

    /**
     * build the mod file given at path, into an executable at outputPath
     */
    int build_mod_file(LabBuildContext& context, const std::string_view& path, chem::string outputPath);

    /**
     * the destructor
     */
    ~LabBuildCompiler();

};