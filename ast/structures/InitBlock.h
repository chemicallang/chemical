// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"
#include <unordered_map>

struct InitBlockInitializerValue {
    Value* value;
};

class InitBlock : public ASTNode {
public:

    std::unordered_map<chem::string_view, InitBlockInitializerValue> initializers;

    /**
     * constructor
     */
    InitBlock(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::InitBlock, parent_node, location) {

    }

    InitBlock* copy(ASTAllocator &allocator) override {
        const auto blk = new (allocator.allocate<InitBlock>()) InitBlock(
            parent(),
            encoded_location()
        );
        for(auto& init : initializers) {
            blk->initializers[init.first] = { init.second.value->copy(allocator) };
        }
        return blk;
    }

    MembersContainer* getContainer();

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};