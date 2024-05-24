// Copyright (c) Qinetik 2024.

#include <string>
#include "ASTProcessorOptions.h"

/**
 * this allows you to control the compilation process
 */
class IGCompilerOptions : public ASTProcessorOptions {
public:

    /**
     * the target triple, which codegen is for
     */
    std::string target_triple;

    /**
     * is the target triple 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    IGCompilerOptions(std::string exe_path, std::string target_triple, bool is64Bit);

};

class Codegen;

/**
 * will compile from the given path
 */
bool compile(Codegen* gen, const std::string& path, IGCompilerOptions* options);