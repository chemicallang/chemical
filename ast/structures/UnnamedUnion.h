// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer, public UnionType {
private:
    bool has_moved = false;
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

    void moved() {
        has_moved = true;
    }

    bool get_has_moved() {
        return has_moved;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    BaseDefMember *copy_member() override;

    VariablesContainer *copy_container() override;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void redeclare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

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

    hybrid_ptr<BaseType> get_value_type() override;

    ASTNode *linked_node() override {
        return this;
    }

    BaseType *copy() const override;

    ValueType value_type() const override {
        return ValueType::Union;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Union;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override {
        return UnionType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override {
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