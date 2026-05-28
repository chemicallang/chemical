// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "Scope.h"

class UnsafeBlock : public ASTNode {
public:

    Scope scope;

    /**
     * optional flag name for lifetime_check or similar features
     */
    chem::string_view flag_name;

    /**
     * constructor
     */
    constexpr UnsafeBlock(
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::UnsafeBlock, parent_node, location), scope(parent_node, location) {

    }

    UnsafeBlock* copy(ASTAllocator &allocator) override {
        const auto blk = new (allocator.allocate<UnsafeBlock>()) UnsafeBlock(
            parent(), encoded_location()
        );
        blk->flag_name = flag_name;
        scope.copy_into(blk->scope, allocator, this);
        return blk;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        scope.code_gen(gen);
    }

#endif

};