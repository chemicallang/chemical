// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class EnumMember : public ASTNode {
public:

    std::string name;
    unsigned int index;

    EnumMember(const std::string& name, unsigned int index) : name(name), index(index) {

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

};