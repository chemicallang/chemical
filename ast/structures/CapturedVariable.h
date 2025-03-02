// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/PointerType.h"

class CapturedVariable : public ASTNode {
public:

    bool capture_by_ref;
    chem::string_view name;
    unsigned int index;
    LambdaFunction *lambda;
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

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    ASTNode *child(const chem::string_view &name) final {
        return linked->child(name);
    }

    ASTNode *child(int index) final {
        return linked->child(index);
    }

    int child_index(const chem::string_view &name) final {
        return linked->child_index(name);
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    BaseType* create_value_type(ASTAllocator &allocator) final;

    BaseType* known_type() final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};