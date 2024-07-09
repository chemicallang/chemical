// Copyright (c) Qinetik 2024.

#include <string>
#include "ASTProcessorOptions.h"
#include "OutputMode.h"

/**
 * this allows you to control the compilation process
 */
class IGCompilerOptions : public ASTProcessorOptions {
public:

    using ASTProcessorOptions::ASTProcessorOptions;

    /**
     * output mode determines the type of files saved
     */
    OutputMode mode;

};

class Codegen;

/**
 * will compile from the given path
 */
bool compile(Codegen* gen, const std::string& path, IGCompilerOptions* options);