// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/UnionType.h"
#include "ast/base/ExtendableMembersContainerNode.h"

class UnionDef : public ExtendableMembersContainerNode, public UnionType {
public:

    ASTNode* parent_node;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    UnionDef(std::string name, ASTNode* parent_node);

    ASTNodeKind kind() override {
        return ASTNodeKind::UnionDecl;
    }

    std::string union_name() override {
        return name;
    }

    VariablesContainer *as_variables_container() override {
        return this;
    }

    VariablesContainer *variables_container() override {
        return this;
    }

    VariablesContainer *copy_container() override;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    const std::string& ns_node_identifier() override {
        return name;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

    [[nodiscard]]
    BaseType *copy() const override;

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    ASTNode *linked_node() override {
        return this;
    }

    bool is_anonymous() override {
        return has_annotation(AnnotationKind::Anonymous);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override {
        return UnionType::llvm_type(gen);
    }

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override {
        return UnionType::llvm_chain_type(gen, values, index);
    }

    llvm::StructType *llvm_union_get_stored_type() override {
        return llvm_struct_type;
    }

    void llvm_union_type_store(llvm::StructType* type) override {
        llvm_struct_type = type;
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