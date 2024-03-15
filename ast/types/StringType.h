// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::String;
    }

    std::string representation() const override {
        return "string";
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override {
        return gen.builder->getInt8PtrTy();
    }
#endif

};