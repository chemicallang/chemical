// Copyright (c) Chemical Language Foundation 2025.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;

    /**
     * constructor
     */
    constexpr ComptimeBlock(
            ASTNode* parent,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ComptimeBlock, parent, location), body(this, location) {

    }

    ComptimeBlock* copy(ASTAllocator &allocator) override {
        const auto blk = new (allocator.allocate<ComptimeBlock>()) ComptimeBlock(
            parent(),
            encoded_location()
        );
        body.copy_into(blk->body, allocator, blk);
        return blk;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};