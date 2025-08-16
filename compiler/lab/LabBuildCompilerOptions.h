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
    OutputMode outMode = OutputMode::Debug;

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