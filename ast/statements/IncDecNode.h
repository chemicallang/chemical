// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/IncDecValue.h"

class IncDecNode : public ASTNode {
public:

    IncDecValue value;

    constexpr IncDecNode(
        Value* value,
        bool increment,
        bool post,
        SourceLocation loc,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::IncDecNode, parent_node, loc), value(value, increment, post, loc) {

    }

    IncDecNode* copy(ASTAllocator &allocator) override {
        const auto node = new (allocator.allocate<IncDecNode>()) IncDecNode(
            value.getValue()->copy(allocator),
            value.increment,
            value.post,
            encoded_location(),
            parent()
        );
        return node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = value.llvm_value(gen, nullptr);
        value.llvm_destruct(gen, inst);
    }

#endif

};