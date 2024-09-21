// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer, public UnionType {
public:

    ASTNode* parent_node;
    CSTToken* token;
    AccessSpecifier specifier;

    UnnamedUnion(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken* cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::UnnamedUnion;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    BaseDefMember* copy_member(ASTAllocator &allocator) override;

    VariablesContainer* copy_container(ASTAllocator &allocator) override;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) override;

    // TODO copy function support unnamed union
    bool requires_copy_fn() override {
        return false;
    }

    // TODO move function support unnamed union
    bool requires_clear_fn() override {
        return false;
    }

    // TODO move function support unnamed union
    bool requires_move_fn() override {
        return false;
    }

    // TODO destructor support unnamed union
    bool requires_destructor() override {
        return false;
    }

    ASTNode *child(const std::string &name) override {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) override {
        return largest_member()->byte_size(is64Bit);
    }

    ASTNode *linked_node() override {
        return this;
    }

    [[nodiscard]]
    BaseType *copy(ASTAllocator& allocator) const override;

    BaseType* known_type() override {
        return this;
    }

    BaseType* create_value_type(ASTAllocator &allocator) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Union;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Union;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override {
        return UnionType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) override {
        return UnionType::llvm_chain_type(gen, values, index);
    }

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
    ) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};