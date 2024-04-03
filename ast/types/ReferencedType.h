// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ReferencedType : public BaseType {
public:

    std::string type;

    ASTNode* linked;

    ReferencedType(const std::string type) : type(type) {}

    void link(SymbolResolver &linker) override;

    ASTNode * linked_node() override;

    bool satisfies(ValueType value_type) const override {
        throw "referenced type cannot determine whether the given value type satisfies";
    }

    std::string representation() const override {
        return type;
    }

    virtual BaseType *copy() const {
        return new ReferencedType(type);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};