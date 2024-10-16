// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/IntType.h"

class EnumMember : public ASTNode {
public:

    std::string name;
    unsigned int index;
    EnumDeclaration* parent_node;
    Value* init_value;
    CSTToken* token;

    EnumMember(
        std::string  name,
        unsigned int index,
        Value* init_value,
        EnumDeclaration* parent_node,
        CSTToken* token
    ) : name(std::move(name)), index(index), init_value(init_value), parent_node(parent_node), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::EnumMember;
    }

    ASTNode *parent() override {
        return (ASTNode*) parent_node;
    }

    void accept(Visitor *visitor) override {

    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    BaseType* create_value_type(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

};