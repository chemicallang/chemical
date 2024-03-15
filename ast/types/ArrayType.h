// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ArrayType : public BaseType {
public:

    std::unique_ptr<BaseType> elem_type;

    unsigned int array_size;

    ArrayType(std::unique_ptr<BaseType> elem_type, unsigned int array_size) : elem_type(std::move(elem_type)),
                                                                              array_size(array_size) {

    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::String;
    }

    std::string representation() const override {
        return "string";
    }

    llvm::Type *llvm_type(Codegen &gen) const override {
        return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
    }

};