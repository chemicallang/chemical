// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class UnreachableStmt : public ASTNode {
public:

    /**
     * constructor
     */
    constexpr UnreachableStmt(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnreachableStmt, parent_node, location) {

    }

    UnreachableStmt* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UnreachableStmt>()) UnreachableStmt(
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

};