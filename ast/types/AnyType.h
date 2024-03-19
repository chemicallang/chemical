// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return true;
    }

    std::string representation() const override {
        return "any";
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override {
        throw std::runtime_error("llvm_type called on any type");
    }
#endif

};