// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "LabBuildCompilerOptions.h"
#include "LabJob.h"
#include "ctpl.h"

class LabBuildContext;

/**
 * lab build compiler, doesn't relate to building a .lab file
 * it provides easy methods to do what can be done with a .lab file
 * The limited api is used to provide command line interface to our compiler applications
 */
class LabBuildCompiler {
public:

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
     * a pointer to current job
     */
    LabJob* current_job;

    /**
     * constructor
     */
    explicit LabBuildCompiler(LabBuildCompilerOptions* options);

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
     * will build the lab file
     */
    int build_lab_file(LabBuildContext& context, const std::string& path);

};