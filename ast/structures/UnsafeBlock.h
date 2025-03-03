// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "Scope.h"

class UnsafeBlock : public ASTNode {
public:

    Scope scope;

    /**
     * constructor
     */
    constexpr UnsafeBlock(
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::UnsafeBlock, parent_node, location), scope(parent_node, location) {

    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    UnsafeBlock* copy(ASTAllocator &allocator) override {
        const auto blk = new (allocator.allocate<UnsafeBlock>()) UnsafeBlock(
            parent(), encoded_location()
        );
        scope.copy_into(blk->scope, allocator, this);
        return blk;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        scope.code_gen(gen);
    }

#endif

};