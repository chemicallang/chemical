// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"
#include <unordered_map>

struct InitBlockInitializerValue {
    bool is_inherited_type;
    Value* value;
};

class InitBlock : public ASTNode {
public:

    Scope scope;
    std::unordered_map<chem::string_view, InitBlockInitializerValue> initializers;
    // the struct container for which init block is for
    ExtendableMembersContainerNode* container;

    /**
     * constructor
     */
    InitBlock(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::InitBlock, parent_node, location), scope(this, location) {

    }

    InitBlock* copy(ASTAllocator &allocator) override {
        const auto blk = new (allocator.allocate<InitBlock>()) InitBlock(
            parent(),
            encoded_location()
        );
        scope.copy_into(blk->scope, allocator, blk);
        for(auto& init : initializers) {
            blk->initializers[init.first] = { init.second.is_inherited_type, init.second.value->copy(allocator) };
        }
        return blk;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};