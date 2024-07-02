// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer, public UnionType {
public:

    ASTNode* parent_node;

    UnnamedUnion(
        std::string name,
        ASTNode* parent_node
    );

    VariablesContainer *variables_container() override {
        return this;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_and_link(SymbolResolver &linker) override;

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

    UnnamedUnion *as_unnamed_union() override {
        return this;
    }

    BaseType *copy() const override;

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const std::string &name
    ) override {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};