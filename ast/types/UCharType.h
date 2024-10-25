// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNType.h"

class UCharType : public IntNType {
public:

    static const UCharType instance;

    using IntNType::IntNType;

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    unsigned int num_bits() const final{
        return 8;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::UChar;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::UChar;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::UChar;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::UChar;
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    UCharType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UCharType>()) UCharType(location);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};