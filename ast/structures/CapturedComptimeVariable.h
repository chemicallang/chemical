// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"

class CapturedComptimeVariable : public ASTNode {
public:

    ASTNode *linked;

    /**
     * constructor
     */
    constexpr CapturedComptimeVariable(
        ASTNode* linked,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::CapturedComptimeVariable, parent_node, location), linked(linked) {

    }

    CapturedComptimeVariable* copy(ASTAllocator &allocator) override {
        const auto var = new (allocator.allocate<CapturedComptimeVariable>()) CapturedComptimeVariable(
            linked,
            parent(),
            encoded_location()
        );
        return var;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Value *llvm_pointer(Codegen &gen);

    inline llvm::Value* loadable_llvm_pointer(Codegen& gen) {
        // stored in a struct, always requires a load
        return llvm_pointer(gen);
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* known_type() {
        return linked->known_type();
    }

};