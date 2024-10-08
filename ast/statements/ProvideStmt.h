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

template<typename T>
void ProvideStmt::put_in(std::unordered_map<std::string, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*)) {
    auto& implicit_args = value_map;
    auto found = implicit_args.find(identifier);
    if(found != implicit_args.end()) {
        // record the previous value
        const auto previous = found->second;
        // set the new value
        found->second = new_value;
        // link the body
        do_body(this, data);
        // set previous value
        found->second = previous;
    } else {
        // set the value
        implicit_args[identifier] = new_value;
        // link the body
        do_body(this, data);
        // remove the value
        implicit_args.erase(identifier);
    }
}