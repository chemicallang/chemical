// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ArrayType : public BaseType {
public:

    std::unique_ptr<BaseType> elem_type;

    int array_size;

    ArrayType(std::unique_ptr<BaseType> elem_type, int array_size) : elem_type(std::move(elem_type)),
                                                                              array_size(array_size) {

    }

    virtual BaseType* copy() const {
        return new ArrayType(std::unique_ptr<BaseType>(elem_type->copy()), array_size);
    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::Array;
    }

    std::string representation() const override {
        return elem_type->representation() + ((array_size == -1) ? "[]" : ("[" + std::to_string(array_size) + "]"));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override {
        return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
    }
#endif

};