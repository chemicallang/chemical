// Copyright (c) Chemical Language Foundation 2025.


#pragma once

#include "ast/structures/Scope.h"
#include "ast/base/Value.h"
#include <unordered_map>

class ProvideStmt : public ASTNode {
public:

    Value* value;
    chem::string_view identifier;
    Scope body;


    /**
     * constructor
     */
    constexpr ProvideStmt(
            Value* value,
            chem::string_view identifier,
            ASTNode* parent,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ProvideStmt, parent, location), value(value), identifier(identifier), body(this, location) {

    }

    /**
     * will allow the caller to put the value
     * in the unordered map, while working with the body
     */
    template<typename T>
    void put_in(std::unordered_map<chem::string_view, T*>& value_map, T* new_value, void* data, void(*do_body)(ProvideStmt*, void*));

    ProvideStmt* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<ProvideStmt>()) ProvideStmt(
            value->copy(allocator),
            identifier,
            parent(),
            encoded_location()
        );
        body.copy_into(stmt->body, allocator, stmt);
        return stmt;
    }

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