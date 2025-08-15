// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/PlacementNewValue.h"

class PlacementNewNode : public ASTNode {
public:

    PlacementNewValue value;

    PlacementNewNode(
        Value* pointer,
        Value* value,
        SourceLocation loc,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::PlacementNewNode, parent_node, loc), value(pointer, value, loc) {

    }

    PlacementNewNode* copy(ASTAllocator &allocator) override {
        const auto node = new (allocator.allocate<PlacementNewNode>()) PlacementNewNode(
            value.pointer->copy(allocator), value.value->copy(allocator),
            encoded_location(),
            parent()
        );
        node->value.setType(value.getType());
        if(node->value.value) {
            // need to set this explicitly
            node->value.ptr_type.type = node->value.value->getType();
        }
        return node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = value.llvm_value(gen);
        value.llvm_destruct(gen, inst);
    }

#endif

};