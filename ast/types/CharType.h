// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNType.h"

class CharType : public IntNType {
public:

    static const CharType instance;

    using IntNType::IntNType;

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 8;
    }

    bool is_unsigned() override {
        return false;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Char;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Char;
    }

    bool satisfies(ASTAllocator& allocator, Value* value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    [[nodiscard]]
    CharType* copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<CharType>()) CharType(token);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};