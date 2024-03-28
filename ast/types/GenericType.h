// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class GenericType : public BaseType {
public:

    std::string base;
    std::unique_ptr<BaseType> type;

    GenericType(std::string base, std::unique_ptr<BaseType> type) : base(std::move(base)), type(std::move(type)) {

    }

    bool satisfies(ValueType value_type) const override {
        return type->satisfies(value_type);
    }

    std::string representation() const override {
        return  '<' + type->representation() + '>';
    }

    virtual BaseType* copy() const {
        return new GenericType(base, std::unique_ptr<BaseType>(type->copy()));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override {
        return type->llvm_type(gen);
    }
#endif

};