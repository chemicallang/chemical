// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/ASTProcessorOptions.h"
#include "compiler/OutputMode.h"

/**
 * this allows you to control the compilation process
 */
class LabBuildCompilerOptions : public ASTProcessorOptions {
public:

    /**
     * the build directory, where everything will be outputted
     */
    std::string build_dir;

    /**
     * use tcc compiler to build and generate executable
     */
#ifdef TCC_BUILD
    bool use_tcc = true;
#else
    bool use_tcc = false;
#endif

    /**
     * caching can be enabled or disabled, when enabled
     * timestamp and other caching related data is generated to build directory
     */
    bool is_caching_enabled = true;

    /**
     * should build lab be cached into an object file
     * or a timestamp be generated for it ?
     */
    bool is_build_lab_caching_enabled = true;

    /**
     * force recompilation of plugins
     */
    bool force_recompile_plugins = false;

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
     * this changes with each job, currently it doesn't change in the middle of the job
     * but we may support that for some modules
     */
    OutputMode out_mode = OutputMode::Debug;

    /**
     * default output mode
     */
    OutputMode def_out_mode = OutputMode::Debug;

    /**
     * plugin mode, plugins are compiled in this mode
     * usually when user changes his mode using --mode we don't want to
     */
#ifdef DEBUG
    OutputMode def_plugin_mode = OutputMode::Debug;
#else
    OutputMode def_plugin_mode = OutputMode::ReleaseFast;
#endif

    /**
     * should the generated c code be minified
     */
    bool minify_c = false;

    /**
     * whether the generated ir should be debuggable ir
     */
    bool debug_ir = false;

    /**
     * lto is on or not, mostly re configured using the output mode above
     * in release mode, it's always on
     */
    bool def_lto_on = false;

    /**
     * also re configured by the output mode
     */
    bool def_assertions_on = false;

#ifdef COMPILER_BUILD
    /**
     * no pie flag allows to control PIE, we simply pass the flag to clang when linking, if enabled
     */
    bool no_pie = false;
#endif

#ifdef COMPILER_BUILD
    /**
     * should the lld be used directly
     */
    bool use_lld = false;
#endif


    /**
     * constructor
     */
    LabBuildCompilerOptions(
        std::string exe_path,
        std::string target_triple,
        std::string build_dir,
        bool is64Bit
    ) : ASTProcessorOptions(std::move(exe_path), std::move(target_triple), is64Bit), build_dir(std::move(build_dir)) {

    }


};