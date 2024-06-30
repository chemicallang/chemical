// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/IntType.h"

class EnumMember : public ASTNode, public IntType {
public:

    std::string name;
    unsigned int index;
    ASTNode* parent_node;

    EnumMember(
        const std::string& name,
        unsigned int index,
        ASTNode* parent_node
    ) : name(name), index(index), parent_node(parent_node) {

    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {

    }

    EnumMember *as_enum_member() override {
        return this;
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen &gen) override;


    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

};