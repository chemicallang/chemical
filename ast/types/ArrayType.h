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

    BaseTypeKind kind() const override {
        return BaseTypeKind::Array;
    }

    bool equals(ArrayType* type) const {
        return type->array_size != array_size && elem_type->is_same(type->elem_type.get());
    }

    bool is_same(BaseType *type) const override {
        return kind() == type->kind() && equals(static_cast<ArrayType*>(type));
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
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};