// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNType.h"

class CharType : public IntNType {
public:

    static const CharType instance;

    using IntNType::IntNType;

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 8;
    }

    bool is_unsigned() final {
        return false;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Char;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Char;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Char;
    }

    [[nodiscard]]
    CharType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<CharType>()) CharType(location);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};