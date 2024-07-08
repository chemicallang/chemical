// Copyright (c) Qinetik 2024.

#include <string>
#include "compiler/ASTProcessorOptions.h"

/**
 * this allows you to control the compilation process
 */
class LabBuildCompilerOptions : public ASTProcessorOptions {
public:

    using ASTProcessorOptions::ASTProcessorOptions;

};

class LabBuildContext;

/**
 * will compile from the given path
 */
bool lab_build(LabBuildContext& context, const std::string& path, LabBuildCompilerOptions* options);