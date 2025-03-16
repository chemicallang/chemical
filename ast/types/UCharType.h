// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNType.h"

class UCharType : public IntNType {
public:

    static const UCharType instance;

    /**
     * constructor
     */
    constexpr UCharType(SourceLocation location) : IntNType(location) {

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

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UCharType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UCharType>()) UCharType(encoded_location());
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};