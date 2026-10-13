// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"

class CapturedVariable : public ASTNode {
public:

    bool capture_by_ref;
    chem::string_view name;
    unsigned int index;
    ASTNode *linked;
    ReferenceType refType;

    /**
     * constructor
     */
    constexpr CapturedVariable(
        chem::string_view name,
        unsigned int index,
        bool capture_by_ref,
        bool mutable_ref,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::CapturedVariable, parent_node, location), name(name), index(index),
        capture_by_ref(capture_by_ref), refType(nullptr, mutable_ref) {

    }

    CapturedVariable* copy(ASTAllocator &allocator) override {
        const auto var = new (allocator.allocate<CapturedVariable>()) CapturedVariable(
            name,
            index,
            capture_by_ref,
            refType.is_mutable,
            parent(),
            encoded_location()
        );
        var->linked = linked;
        return var;
    }


    BaseType* known_type();

};