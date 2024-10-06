// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ArrayType : public TokenizedBaseType {
public:

    BaseType* elem_type;
    int array_size;
    CSTToken* token;

    ArrayType(
        BaseType* elem_type,
        int array_size,
        CSTToken* token
    ) : elem_type(elem_type), array_size(array_size), TokenizedBaseType(token) {

    }

    uint64_t byte_size(bool is64Bit) override {
        if(array_size == -1) {
            throw std::runtime_error("array size not known, byte size required");
        } else {
            return array_size * elem_type->byte_size(is64Bit);
        }
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ASTAllocator& allocator, Value* value) override;

    bool link(SymbolResolver &linker) override;

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const override {
        return elem_type->copy(allocator);
    }

//    hybrid_ptr<BaseType> get_child_type() override {
//        return hybrid_ptr<BaseType> { elem_type.get(), false };
//    }

    BaseType* known_child_type() override {
        return elem_type;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Array;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Array;
    }

    bool equals(ArrayType *type) const {
        return type->array_size == array_size && elem_type->is_same(type->elem_type);
    }

    bool is_same(BaseType *type) override {
        return kind() == type->kind() && equals(static_cast<ArrayType *>(type));
    }

    [[nodiscard]]
    ArrayType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<ArrayType>()) ArrayType(elem_type->copy(allocator), array_size, token);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Array;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

#endif

};