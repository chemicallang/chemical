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
     * the actual identifier / access chain value destruct[array_value] ptr; <--- ptr is the one
     */
    Value* identifier;
    /**
     * array value is the one in brackets like destruct[array_value] ptr;
     */
    Value* array_value;
    /**
     * if the statement has brackets destruct[] ptr;
     */
    bool is_array;
    /**
     * should the pointer be free'd after
     */
    bool free_after = false;

    /**
     * constructor
     */
    constexpr DestructStmt(
        Value* array_value,
        Value* value,
        bool is_array,
        bool free_after,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::DeleteStmt, parent_node, location), array_value(array_value), identifier(value), is_array(is_array), free_after(free_after) {

    }

    inline bool getFreeAfter() {
        return free_after;
    }

    inline void setFreeAfter(bool value) {
        free_after = value;
    }

    DestructData get_data(ASTAllocator& allocator);

    DestructStmt* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<DestructStmt>()) DestructStmt(
            array_value ? array_value->copy(allocator) : nullptr,
            identifier->copy(allocator),
            is_array,
            free_after,
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};