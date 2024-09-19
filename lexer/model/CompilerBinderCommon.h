// Copyright (c) Qinetik 2024.

#pragma once

#include "CompilerBinder.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/GlobalInterpretScope.h"

class CompilerBinderCommon : public CompilerBinder {
public:

    /**
     * a global interpret scope is used, when symbol resolving
     * or generating code
     */
    GlobalInterpretScope global;

    /**
     * diagnoser that will take errors
     */
    CSTDiagnoser* diagnoser;

    /**
     * A single converter is used to convert the tokens given to ASTNodes
     */
    CSTConverter converter;

    /**
     * resolves symbols inside the code
     */
    SymbolResolver resolver;

    /**
     * everything that is ever collected under a name, ends up here
     */
    std::unordered_map<std::string, std::vector<std::unique_ptr<ASTNode>>> collected;

    /**
     * when nodes for a cbi are collected, they are put in this map
     */
    std::unordered_map<std::string, std::vector<ASTNode*>> cbi;

    /***
     * parses tokens into nodes
     */
    std::vector<std::unique_ptr<ASTNode>> parse(std::vector<CSTToken*>& tokens);

    /**
     * constructor
     */
    explicit CompilerBinderCommon(
        CSTDiagnoser* diagnoser,
        ASTAllocator<>& job_allocator,
        ASTAllocator<>& mod_allocator
    );

    /**
     * will create a cbi, cause error if it already exists
     */
    void create_cbi(const std::string &cbi_name) override;

    /**
     * imports a collected container into cbi
     */
    void import_container(const std::string &cbi_name, const std::string &container) override;

    /**
     * will collect node globally into global map
     */
    void collect(const std::string& name, std::vector<CSTToken*> &tokens, bool err_no_found) override;


};