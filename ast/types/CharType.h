// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNType.h"

class CharType : public IntNType {
public:

    static const CharType instance;

    /**
     * constructor
     */
    constexpr CharType(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Char;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 8;
    }

    bool is_unsigned() final {
        return false;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    CharType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<CharType>()) CharType(encoded_location());
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) final;
#endif

};