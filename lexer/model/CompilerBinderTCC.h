// Copyright (c) Qinetik 2024.

#pragma once

#include "CompilerBinder.h"
#include "libtcc.h"
#include "cst/base/CSTConverter.h"
#include "preprocess/2cASTVisitor.h"
#include "compiler/SymbolResolver.h"
#include <mutex>

/**
 * compiler binder based on tiny c compiler
 */
class CompilerBinderTCC : public CompilerBinder {
public:

    /**
     * diagnoser that will take errors
     */
    CSTDiagnoser* diagnoser;

    /**
     * TCC state
     */
    TCCState* state = nullptr;

    /**
     * A single converter is used to convert the tokens given to ASTNodes
     */
    CSTConverter converter;

    /**
     * resolves symbols inside the code
     */
    SymbolResolver resolver;

    /**
     * A single To C AST Visitor is used to translate converter ASTNodes to 'C'
     */
    ToCAstVisitor translator;

    /**
     * a start token is cached, to provide error reporting
     */
    CSTToken* cached_start_token;

    /**
     * a end token is cached, to provide error reporting
     */
    CSTToken* cached_end_token;

    /**
     * a mutex is locked when compiling c to ensure a single compilation
     */
    std::mutex c_compile_mutex;

    /**
     * constructor
     */
    explicit CompilerBinderTCC(CSTDiagnoser* diagnoser);

    /**
     * initializes
     */
    void init() override;

    /**
     * overrides the compile function
     */
    bool compile(std::vector<std::unique_ptr<CSTToken>>& tokens) override;

    /**
     * provides a pointer to function
     */
    void* provide_func(const std::string& funcName) override;

    /**
     * reset new file, would change the error token
     */
    void reset_new_file() override;

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinderTCC() override;

};