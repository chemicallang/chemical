// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ArrayType : public BaseType {
public:

    std::unique_ptr<BaseType> elem_type;

    unsigned int array_size;

    ArrayType(std::unique_ptr<BaseType> elem_type, unsigned int array_size) : elem_type(elem_type),
                                                                              array_size(array_size) {

    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::String;
    }

    std::string representation() const override {
        return "string";
    }

    llvm::Type *llvm_type(Codegen &gen) const override {
        throw std::runtime_error("llvm string type is not supported");
    }

};