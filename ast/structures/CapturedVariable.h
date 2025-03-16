// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/PointerType.h"

class CapturedVariable : public ASTNode {
public:

    bool capture_by_ref;
    chem::string_view name;
    unsigned int index;
    ASTNode *linked;
    PointerType ptrType;

    /**
     * constructor
     */
    constexpr CapturedVariable(
        chem::string_view name,
        unsigned int index,
        bool capture_by_ref,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::CapturedVariable, parent_node, location), name(name), index(index),
        capture_by_ref(capture_by_ref), ptrType(nullptr, location) {

    }

    bool declare_and_link(SymbolResolver& linker);

    inline void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        declare_and_link(linker);
    }

    ASTNode *child(const chem::string_view &name) final {
        return linked->child(name);
    }

    CapturedVariable* copy(ASTAllocator &allocator) override {
        const auto var = new (allocator.allocate<CapturedVariable>()) CapturedVariable(
            name,
            index,
            capture_by_ref,
            parent(),
            encoded_location()
        );
        var->linked = linked;
        return var;
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* known_type() final;

};