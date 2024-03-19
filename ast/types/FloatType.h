// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Float;
    }

    std::string representation() const override {
        return "float";
    }

    virtual BaseType* copy() const {
        return new FloatType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override {
        return gen.builder->getFloatTy();
    }
#endif

};