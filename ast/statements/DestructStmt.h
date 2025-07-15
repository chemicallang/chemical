// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class ExtendableMembersContainerNode;

struct DestructData {
    ExtendableMembersContainerNode* parent_node;
    FunctionDeclaration* destructor_func;
    uint64_t array_size; // 0 if not known
};

class DestructStmt : public ASTNode {
public:

    /**
     * if the statement has brackets destruct[] ptr;
     */
    bool is_array;
    /**
     * the actual identifier / access chain value destruct[array_value] ptr; <--- ptr is the one
     */
    Value* identifier;
    /**
     * array value is the one in brackets like destruct[array_value] ptr;
     */
    Value* array_value;

    /**
     * constructor
     */
    constexpr DestructStmt(
        Value* array_value,
        Value* value,
        bool is_array,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::DeleteStmt, parent_node, location), array_value(array_value), identifier(value), is_array(is_array) {

    }

    DestructData get_data(ASTAllocator& allocator);

    DestructStmt* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<DestructStmt>()) DestructStmt(
            array_value ? array_value->copy(allocator) : nullptr,
            identifier->copy(allocator),
            is_array,
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};