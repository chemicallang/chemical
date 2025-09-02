// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNType.h"

class UCharType : public IntNType {
public:

    /**
     * constructor
     */
    constexpr UCharType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UChar;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final{
        return 8;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    UCharType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UCharType>()) UCharType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};