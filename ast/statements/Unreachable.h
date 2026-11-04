// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class UnreachableStmt : public ASTNode {
public:

    /** Compiler-generated loop placeholders are not user traps and do not
     * require an unsafe boundary. */
    bool is_generated = false;

    /**
     * constructor
     */
    constexpr UnreachableStmt(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnreachableStmt, parent_node, location) {

    }

    UnreachableStmt* copy(ASTAllocator& allocator) final {
        auto* copied = new (allocator.allocate<UnreachableStmt>()) UnreachableStmt(
            parent(),
            encoded_location()
        );
        copied->is_generated = is_generated;
        return copied;
    }


};