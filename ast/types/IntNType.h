// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/base/IntNTypeKind.h"

class IntNType : public BaseType {
public:

    /**
     * constructor
     */
    inline constexpr IntNType() : BaseType(BaseTypeKind::IntN) {

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
    virtual unsigned int num_bits(bool is64Bit) const = 0;

    /**
     * to signed intN type, if this is unsigned, otherwise this is returned
     */
    IntNType* to_signed(ASTAllocator& allocator);

    /**
     * check if this is a character type
     */
    bool is_char_or_uchar_type() {
        const auto k = IntNKind();
        return k == IntNTypeKind::Char || k == IntNTypeKind::UChar;
    }

    /**
     * creates a Value of this type, but with the given value
     */
    virtual Value* create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) = 0;

    /**
     * types that are larger in bits or smaller can satisfy each other as long as they are same signed
     * however maybe we should have a diagnostic for this
     */
    bool satisfies(IntNType* type) {
        const auto this_unsigned = is_unsigned();
        const auto other_unsigned = type->is_unsigned();
        return (this_unsigned && other_unsigned) || (!this_unsigned && !other_unsigned);
    }

    /**
     * check if satisfies
     */
    bool satisfies(BaseType *type) {
        return type->kind() == BaseTypeKind::IntN && satisfies(type->as_intn_type_unsafe());
    }

    /**
     * check if value satisfies
     */
    bool satisfies(ASTAllocator &allocator, Value *value, bool assignment) override;

    /**
     * this checks whether this int n type is equal to this other int n type
     */
    bool equals(IntNType* type) {
        return type->IntNKind() == IntNKind();
    }

    /**
     * converts a kind to unsigned kind
     */
    static IntNTypeKind to_unsigned_kind(IntNTypeKind k) {
        auto otherK = static_cast<int>(k);
        if(otherK < static_cast<int>(IntNTypeKind::UnsignedStart)) {
            return static_cast<IntNTypeKind>(otherK + static_cast<int>(IntNTypeKind::UnsignedStart));
        } else {
            return k;
        }
    }

    /**
     * checks if this type is bigger in terms of bit width, than the given type
     */
    bool greater_than_in_bits(IntNType* type) {
        return to_unsigned_kind(IntNKind()) > to_unsigned_kind(type->IntNKind());
    }

    [[nodiscard]]
    BaseTypeKind kind() const {
        return BaseTypeKind::IntN;
    }

    bool is_same(BaseType *type) {
        return type->kind() == kind() && equals(type->as_intn_type_unsafe());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

#endif

};