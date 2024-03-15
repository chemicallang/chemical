// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class Int32Type : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Int;
    }

    std::string representation() const override {
        return "int32";
    }

    llvm::Type *llvm_type(Codegen &gen) const override {
        return gen.builder->getInt32Ty();
    }

};