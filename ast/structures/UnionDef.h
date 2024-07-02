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

    VariablesContainer *variables_container() override {
        return this;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    std::string ns_node_identifier() override {
        return name;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseType *copy() const override;

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

    UnionDef *as_union_def() override {
        return this;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
    ) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};