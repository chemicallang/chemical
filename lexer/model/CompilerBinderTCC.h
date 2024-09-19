// Copyright (c) Qinetik 2024.

#pragma once

#include "CompilerBinder.h"
#include "libtcc.h"
#include "cst/base/CSTConverter.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "compiler/SymbolResolver.h"
#include "CompilerBinderCommon.h"
#include <mutex>

/**
 * compiler binder based on tiny c compiler
 */
class CompilerBinderTCC : public CompilerBinderCommon {
public:

    /**
     * A single To C AST Visitor is used to translate converter ASTNodes to 'C'
     */
    ToCAstVisitor translator;

    /**
     * container a map between cbi_name and tcc state
     */
    std::unordered_map<std::string, TCCState*> compiled;

    /**
     * path to current executable, resources required by tcc are located relative to it
     */
    std::string exe_path;

    /**
     * constructor
     */
    explicit CompilerBinderTCC(
        CSTDiagnoser* diagnoser,
        std::string exe_path,
        ASTAllocator<>& job_allocator,
        ASTAllocator<>& mod_allocator
    );

    /**
     * will compile the following cbi
     */
    bool compile(const std::string &cbi_name) override;

    /**
     * provides a pointer to function contained inside cbi
     */
    void* provide_func(const std::string& cbi_name, const std::string& funcName) override;

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinderTCC() override;

};