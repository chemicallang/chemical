// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "LabBuildCompilerOptions.h"
#include "LabJob.h"
#include "ctpl.h"
#include "parser/model/CompilerBinder.h"
#include "cst/LocationManager.h"
#include "preprocess/ImportPathHandler.h"
#include "compiler/mangler/NameMangler.h"

class ASTAllocator;

class LabBuildContext;

class CTranslator;

struct GlobalContainer;

class CmdOptions;

class ToCAstVisitor;

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
     * modules that have been generated are stored on this map
     * so we can reuse them for different jobs, the values
     * are absolute paths to their object or bitcode files
     */
    std::unordered_map<LabModule*, std::string> generated;

    /**
     * the build context that is being used to build
     */
    LabBuildContext* build_context = nullptr;

    /**
     * when given, we check for any command line options that configure the code generator
     */
    CmdOptions* cmd = nullptr;

    /**
     * the global container contains namespaces like std and compiler
     */
    GlobalContainer* container = nullptr;

    /**
     * a pointer to current job
     */
    LabJob* current_job;

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
    explicit LabBuildCompiler(CompilerBinder& binder, LabBuildCompilerOptions* options);

    /**
     * set the cmd options to allow checking for any command line options given to configure the code generation process
     */
    void set_cmd_options(CmdOptions* ptr) {
        cmd = ptr;
    }

    /**
     * this prepares the build compiler for a session of multiple jobs
     * after performing the jobs, we expected that manager, or allocators provided
     * will go out of scope and die, which means we'll have pointers to these freed objects
     * so that's why prepare should be called, for each session of multiple jobs
     */
    void prepare(
            ASTAllocator* const job_allocator,
            ASTAllocator* const mod_allocator,
            ASTAllocator* const file_allocator
    );

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
        std::stringstream& output_ptr,
        LabJobCBI* cbiJob
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
     * processes modules, generates code, or all modules required for linking but doesn't link
     * it also calls appropriate method process job tcc or process job gen
     */
    int process_modules(LabJob* job) {
#ifdef COMPILER_BUILD
        const bool use_tcc = options->use_tcc || job->type == LabJobType::ToCTranslation || job->type == LabJobType::CBI;
        return use_tcc ? (
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
    int link(std::vector<chem::string>& result, const std::string& path);

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
     * filter will be used, to perform jobs that are of interest
     */
    int do_jobs_filtering(bool(*filter)(LabBuildCompiler*, LabJob*));

    /**
     * will build the lab file and return the callable tcc state
     */
    TCCState* built_lab_file(LabBuildContext& context, const std::string& path);

    /**
     * will build the lab file
     */
    int build_lab_file(LabBuildContext& context, const std::string& path);

    /**
     * the destructor
     */
    ~LabBuildCompiler();

};