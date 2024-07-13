// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "compiler/ASTProcessorOptions.h"
#include "compiler/OutputMode.h"
#include "LabJob.h"
#include "ctpl.h"

/**
 * this allows you to control the compilation process
 */
class LabBuildCompilerOptions : public ASTProcessorOptions {
public:

    using ASTProcessorOptions::ASTProcessorOptions;

    /**
     * if left empty, by default a build folder relative to build.lab will be chosen with name build
     */
    std::string build_folder;

    /**
     * use tcc compiler to build and generate executable
     */
#ifdef TCC_BUILD
    bool use_tcc = true;
#else
    bool use_tcc = false;
#endif

    /**
     * will force use object file format
     * // TODO make this by default false, once our bitcode generation is valid
     * // Currently set to true, so object files are emitted
     */
#ifdef TCC_BUILD
    bool use_mod_obj_format = true;
#else
    bool use_mod_obj_format = true;
#endif
    /**
     * default output mode
     */
    OutputMode def_mode = OutputMode::Debug;

    /**
     * lto is on or not, mostly re configured using the output mode above
     * in release mode, it's always on
     */
    bool def_lto_on = false;

    /**
     * also re configured by the output mode
     */
    bool def_assertions_on = false;

};

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
     * does job to translate to C
     */
    int do_to_c_job(LabJob* job);

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