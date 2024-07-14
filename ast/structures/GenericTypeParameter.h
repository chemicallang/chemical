// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class GenericTypeParameter : public ASTNode {
public:

    std::string identifier;
    std::unique_ptr<BaseType> def_type;
    std::vector<std::unique_ptr<BaseType>> usage;
    int16_t active_iteration = -1; // <-- index of active type in usage vector
    FunctionDeclaration* parent_node;

    /**
     * constructor
     */
    GenericTypeParameter(std::string identifier, std::unique_ptr<BaseType> def_type, FunctionDeclaration* parent_node);

    void declare_and_link(SymbolResolver &linker) override;

    void register_usage(std::unique_ptr<BaseType> type);

    ASTNode *parent() override {
        return (ASTNode*) parent_node;
    }

    void accept(Visitor *visitor) override {
        // do nothing
    }

};