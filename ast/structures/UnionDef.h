// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"

class UnionDef : public ExtendableMembersContainerNode {
public:

    std::string name;

#ifdef COMPILER_BUILD
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    UnionDef(std::string name);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type* largest_member_type(Codegen &gen);

    llvm::StructType* get_struct_type(Codegen &gen);

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};