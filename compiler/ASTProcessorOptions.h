// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

/**
 * processor options, anything that processes the AST
 * for compilation can use these options
 */
class ASTProcessorOptions {
public:

    /**
     * benchmark the entire compilation process
     */
    bool benchmark = false;

    /**
     * benchmark the compilation process of each file
     */
    bool benchmark_files = false;

    /**
     * benchmark the compilation process of each module
     */
    bool benchmark_modules = false;

    /**
     * verbose
     */
    bool verbose = false;

    /**
     * pass verbose to link
     */
    bool verbose_link = false;

    /**
     * allows to ignore errors during compilation
     */
    bool ignore_errors = false;

    /**
     * this is turned on when user supplies '-g' in the command line
     * so we should put debug info
     */
    bool debug_info = false;

    /**
     * emit the translated c
     */
    bool emit_c = false;

    /**
     * stops if finds errors in a single file, printing less errors to console
     */
    bool stop_on_file_error = true;

    /**
     * is target system 64Bit
     */
    bool is64Bit;

    /**
     * the target triple, which codegen is for
     */
    std::string target_triple;

    /**
     * path to resources, give only if import's c files
     */
    std::string resources_path;

    /**
     * path to the current compiler executable
     */
    std::string exe_path;

    /**
     * constructor
     */
    ASTProcessorOptions(
            std::string exe_path,
            std::string target_triple,
            bool is64Bit
    ) : exe_path(std::move(exe_path)), target_triple(std::move(target_triple)), is64Bit(is64Bit) {

    }

    std::string get_resources_path();

};