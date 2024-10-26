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
    SourceLocation location;

    /**
     * constructor
     */
    ProvideStmt(
        Value* value,
        std::string identifier,
        Scope body,
        ASTNode* parent,
        SourceLocation location
    );

    /**
     * will allow the caller to put the value
     * in the unordered map, while working with the body
     */
    template<typename T>
    void put_in(std::unordered_map<std::string, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*));

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNode* parent() final {
        return parent_node;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ProvideStmt;
    }

    void declare_and_link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

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