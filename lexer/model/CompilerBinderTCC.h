// Copyright (c) Qinetik 2024.

#pragma once

#include "CompilerBinder.h"
#include "libtcc.h"
#include <mutex>

/**
 * compiler binder based on tiny c compiler
 */
class CompilerBinderTCC : public CompilerBinder {
public:

    /**
     * container a map between cbi_name and tcc state
     */
    std::unordered_map<std::string, TCCState*> compiled;

    /**
     * a map between absolute file paths, and public symbols inside them
     */
    std::unordered_map<std::string, std::unordered_map<std::string, void*>> symbol_maps;

    /**
     * a map between interface names like Lexer, SourceProvider and their actual symbols
     * these symbols correspond to function pointers in the our source code
     */
    std::unordered_map<std::string, std::unordered_map<std::string, void*>> interface_maps;

    /**
     * diagnostics during compilation of c files
     */
    std::vector<std::string> diagnostics;

    /**
     * path to current executable, resources required by tcc are located relative to it
     */
    std::string exe_path;

    /**
     * constructor
     */
    explicit CompilerBinderTCC(std::string exe_path);

    /**
     * following c program will be compiled as the cbi
     */
    BinderResult compile(
        const std::string& cbi_name,
        const std::string& program,
        CBIData& cbiData,
        std::vector<std::string_view>& imports,
        std::vector<std::string_view>& current_files,
        ASTProcessor& processor
    ) override;

    /**
     * import compiler interface with the given name
     */
    bool import_compiler_interface(const std::string& name, TCCState* state);

    /**
     * provides a pointer to function contained inside cbi
     */
    void* provide_func(const std::string& cbi_name, const std::string& funcName) override;

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinderTCC() override;

};