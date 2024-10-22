// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "LabBuildCompilerOptions.h"
#include "LabJob.h"
#include "ctpl.h"
#include "lexer/model/CompilerBinder.h"

class ASTAllocator;

class LabBuildContext;

class CTranslator;

struct GlobalContainer;

/**
 * lab build compiler, doesn't relate to building a .lab file
 * it provides easy methods to do what can be done with a .lab file
 * The limited api is used to provide command line interface to our compiler applications
 */
class LabBuildCompiler {
public:

    /**
     * compiler binder is used to bind compiler functions with user source code
     */
    CompilerBinder& binder;

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
    LabBuildContext* build_context;

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
     * processes modules, generates code, or all modules required for linking but doesn't link
     */
    int process_modules(LabJob* job);

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