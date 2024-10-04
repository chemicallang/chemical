// Copyright (c) Qinetik 2024.


#pragma once

#include "ast/structures/Scope.h"
#include <unordered_map>

class ProvideStmt : public ASTNode {
public:

    Value* value;
    std::string identifier;
    Scope body;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ProvideStmt(
        Value* value,
        std::string identifier,
        Scope body,
        ASTNode* parent,
        CSTToken* token
    );

    /**
     * will allow the caller to put the value
     * in the unordered map, while working with the body
     */
    template<typename T>
    void put_in(std::unordered_map<std::string, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*));

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    CSTToken* cst_token() override {
        return token;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ProvideStmt;
    }

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};