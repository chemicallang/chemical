// Copyright (c) Qinetik 2024.

#include <string>

/**
 * this allows you to control the compilation process
 */
class IGCompilerOptions {
public:

    /**
     * by default compiles depth first
     */
    bool depth_first = true;

    /**
     * benchmark the compilation process
     */
    bool benchmark = false;

    /**
     * print the import graph
     */
    bool print_ig = false;

    /**
     * print the representation of files (ASTNodes)
     */
    bool print_representation = false;

    /**
     * print the tokens lexed
     */
    bool print_cst = false;

    /**
     * verbose
     */
    bool verbose = false;

    /**
     * path to resources, give only if import's c files
     */
    std::string resources_path;

    /**
     * path to the current compiler executable
     */
    std::string exe_path;

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