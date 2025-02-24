// Copyright (c) Chemical Language Foundation 2025.


#pragma once

#include "ast/structures/Scope.h"
#include <unordered_map>

class ProvideStmt : public ASTNode {
public:

    Value* value;
    chem::string_view identifier;
    Scope body;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ProvideStmt(
        Value* value,
        chem::string_view identifier,
        Scope body,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ProvideStmt, location), value(value), identifier(identifier), body(std::move(body)), parent_node(parent) {

    }

    /**
     * will allow the caller to put the value
     * in the unordered map, while working with the body
     */
    template<typename T>
    void put_in(std::unordered_map<chem::string_view, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*));

    ASTNode* parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};

template<typename T>
void ProvideStmt::put_in(std::unordered_map<chem::string_view, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*)) {
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