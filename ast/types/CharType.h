// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class CharType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Char;
    }

    std::string representation() const override {
        return "char";
    }

    llvm::Type *llvm_type(Codegen &gen) const override {
        return gen.builder->getInt8Ty();
    }

};