// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNType.h"

class UCharType : public IntNType {
public:

    static const UCharType instance;

    using IntNType::IntNType;

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    unsigned int num_bits() const override{
        return 8;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::UChar;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::IntN;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::UChar;
    }

    bool satisfies(ASTAllocator& allocator, Value* value) override;

    Value *create(int64_t value) override;

    [[nodiscard]]
    UCharType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<UCharType>()) UCharType(token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};