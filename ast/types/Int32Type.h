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

    virtual BaseType* copy() const {
        return new Int32Type();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};