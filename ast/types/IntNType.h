// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/IntNTypeKind.h"

class IntNType : public BaseType {
public:

    /**
     * constructor
     */
    inline IntNType(SourceLocation location) : BaseType(BaseTypeKind::IntN, location) {

    }

    /**
     * get the int n type kind
     */
    virtual IntNTypeKind IntNKind() const = 0;

    /**
     * is this int n type unsigned
     */
    virtual bool is_unsigned() = 0;

    /**
     * the number of bits, int means int32 which has 32 bits
     */
    [[nodiscard]]
    virtual unsigned int num_bits() const = 0;

    /**
     * check if this is character type
     */
    bool is_char_type() {
        return num_bits() == 8 && !is_unsigned();
    }

    /**
     * check if this is character type
     */
    bool is_uchar_type() {
        return num_bits() == 8 && is_unsigned();
    }

    /**
     * helper
     */
    inline unsigned int number() const {
        return num_bits();
    }

    /**
     * creates a Value of this type, but with the given value
     */
    virtual Value* create(ASTAllocator& allocator, uint64_t value) = 0;

    bool satisfies(BaseType *type);

    [[nodiscard]]
    BaseTypeKind kind() const {
        return BaseTypeKind::IntN;
    }

    bool is_same(BaseType *type) {
        return type->kind() == kind() && ((IntNType*) type)->num_bits() == num_bits() && ((IntNType*) type)->is_unsigned() == is_unsigned();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};