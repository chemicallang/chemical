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

    uint64_t byte_size(bool is64Bit) final {
        if(array_size == -1) {
            throw std::runtime_error("array size not known, byte size required");
        } else {
            return array_size * elem_type->byte_size(is64Bit);
        }
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(BaseType *type) final;

    bool link(SymbolResolver &linker) final;

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final {
        return elem_type->copy(allocator);
    }

//    hybrid_ptr<BaseType> get_child_type() final {
//        return hybrid_ptr<BaseType> { elem_type.get(), false };
//    }

    BaseType* known_child_type() final {
        return elem_type;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Array;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Array;
    }

    bool equals(ArrayType *type) const {
        return type->array_size == array_size && elem_type->is_same(type->elem_type);
    }

    bool is_same(BaseType *type) final {
        return kind() == type->kind() && equals(static_cast<ArrayType *>(type));
    }

    [[nodiscard]]
    ArrayType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ArrayType>()) ArrayType(elem_type->copy(allocator), array_size, token);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Array;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

#endif

};