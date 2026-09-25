// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/AccessChain.h"

class AccessChainNode : public ASTNode {
public:

    AccessChain chain;

    constexpr AccessChainNode(
        SourceLocation loc,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::AccessChainNode, parent_node, loc), chain(loc) {

    }

    AccessChainNode* copy(ASTAllocator &allocator) override {
        const auto node = new (allocator.allocate<AccessChainNode>()) AccessChainNode(
            encoded_location(),
            parent()
        );
        node->chain.setType(chain.getType());
        for(const auto value : chain.values) {
            node->chain.values.emplace_back((Value*) value->copy(allocator));
        }
        return node;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = chain.llvm_value(gen, nullptr);
        // `f().member` destroys the call's temporary (and with it the member)
        // while the chain is loaded, so the chain's value is an alias into that
        // destroyed temporary and must not be destroyed a second time
        if(chain.is_alias_into_destroyed_temp()) {
            return;
        }
        chain.llvm_destruct(gen, inst);
    }

#endif

};