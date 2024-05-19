// Copyright (c) Qinetik 2024.

#include <string>

/**
 * this allows you to control the compilation process
 */
class ToCTranslatorOptions {
public:

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
     * the path to the output, this is a path to a directory
     */
    std::string output_path;

    /**
     * is the target triple 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    ToCTranslatorOptions(std::string exe_path, std::string output_path, bool is64Bit);

};

bool translate(const std::string& path, ToCTranslatorOptions* options);