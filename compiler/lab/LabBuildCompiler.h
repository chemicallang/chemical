// Copyright (c) Qinetik 2024.

#include <string>
#include "compiler/ASTProcessorOptions.h"
#include "compiler/OutputMode.h"

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
 * will compile from the given path
 */
int lab_build(LabBuildContext& context, const std::string& path, LabBuildCompilerOptions* options);