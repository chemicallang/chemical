// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class ArrayType : public BaseType {
private:

    uint64_t array_size = 0;

public:

    BaseType* elem_type;
    Value* array_size_value;

    inline constexpr ArrayType(
        BaseType* elem_type,
        Value* array_size_val,
        SourceLocation location
    ) : BaseType(BaseTypeKind::Array, location), elem_type(elem_type), array_size_value(array_size_val) {

    }

    inline constexpr ArrayType(
            BaseType* elem_type,
            uint64_t array_size,
            SourceLocation location
    ) : BaseType(BaseTypeKind::Array, location), elem_type(elem_type), array_size_value(nullptr), array_size(array_size) {

    }

    inline uint64_t get_array_size() {
        return array_size;
    }

    inline void set_array_size(uint64_t size) {
        array_size = size;
    }

    inline bool has_array_size() {
        return get_array_size() != 0;
    }

    inline bool has_no_array_size() {
        return get_array_size() == 0;
    }

    uint64_t byte_size(bool is64Bit) final {
        if(has_no_array_size()) {
            throw std::runtime_error("array size not known, byte size required");
        } else {
            return array_size * elem_type->byte_size(is64Bit);
        }
    }

    bool satisfies(BaseType *type) final;

    bool link(SymbolResolver &linker) final;

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final {
        return elem_type->copy(allocator);
    }

    BaseType* known_child_type() final {
        return elem_type;
    }

    bool equals(ArrayType *type) const {
        return type->array_size == array_size && elem_type->is_same(type->elem_type);
    }

    bool is_same(BaseType *type) final {
        return kind() == type->kind() && equals(static_cast<ArrayType *>(type));
    }

    [[nodiscard]]
    ArrayType* copy(ASTAllocator& allocator) const final {
        const auto t = new (allocator.allocate<ArrayType>()) ArrayType(elem_type->copy(allocator), array_size_value, encoded_location());
        t->array_size = array_size;
        return t;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

#endif

};