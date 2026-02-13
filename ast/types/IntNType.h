// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/base/IntNTypeKind.h"

class TypeBuilder;

struct TargetData;

class IntNType : public BaseType {
private:

    /**
     * the kind used by the user
     */
    IntNTypeKind _kind;

    /**
     * is the type unsigned
     */
    bool _is_unsigned;

public:

    /**
     * constructor
     */
    inline constexpr IntNType(
            IntNTypeKind kind
    ) : BaseType(BaseTypeKind::IntN), _kind(kind), _is_unsigned(::is_unsigned(kind)) {

    }

    /**
     * constructor
     */
    inline constexpr IntNType(
            IntNTypeKind kind,
            bool is_unsigned
    ) : BaseType(BaseTypeKind::IntN), _kind(kind), _is_unsigned(is_unsigned) {

    }

    /**
     * get the int n type kind
     */
    inline IntNTypeKind IntNKind() const noexcept {
        return _kind;
    }

    /**
     * is this int n type unsigned
     */
    inline bool is_unsigned() const noexcept {
        return _is_unsigned;
    }

    /**
     * the number of bits, int means int32 which has 32 bits
     */
    [[nodiscard]]
    unsigned int num_bits(TargetData& target) const noexcept;

    /**
     * get the byte size of this type
     */
    [[nodiscard]]
    uint64_t byte_size(TargetData& target) final;

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
    Value* create(
            ASTAllocator& allocator,
            TypeBuilder& typeBuilder,
            uint64_t value,
            SourceLocation loc
    );

    /**
     * check if the type satisfies this int n type when in pointer / reference
     * *mut u8 doesn't satisfy *mut u64
     */
    bool pointee_satisfies(IntNType* type) {
        if(_kind != type->_kind) {
            return false;
        }
        if(is_unsigned() != type->is_unsigned()) {
            return false;
        }
        return true;
    }

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
    bool satisfies(BaseType *type);

    /**
     * check if value satisfies
     */
    bool satisfies(Value *value, bool assignment) override;

    /**
     * this checks whether this int n type is equal to this other int n type
     */
    inline bool equals(IntNType* type) const noexcept {
        return type->IntNKind() == IntNKind();
    }

    /**
     * copy the intn type
     */
    BaseType* copy(ASTAllocator &allocator) final {
        // why does this return itself (without copying)
        // because the type exists in type builder
        // it is initialized once in the type builder
        // this will never be copied
        return this;
    }

    /**
     * check if the given int n type kind is unsigned
     */
    static inline bool is_unsigned(IntNTypeKind k) {
        return static_cast<int>(k) >= static_cast<int>(IntNTypeKind::UnsignedStart);
    }

    /**
     * check if the given int n type kind is signed
     */
    static inline bool is_signed(IntNTypeKind k) {
        return static_cast<int>(k) < static_cast<int>(IntNTypeKind::UnsignedStart);
    }

    /**
     * converts a kind to unsigned kind
     */
    static IntNTypeKind to_unsigned_kind(IntNTypeKind k) {
        return is_signed(k) ? static_cast<IntNTypeKind>(static_cast<int>(k) + static_cast<int>(IntNTypeKind::UnsignedStart)): k;
    }

    /**
     * converts a kind to unsigned kind
     */
    static IntNTypeKind to_signed_kind(IntNTypeKind k) {
        return is_unsigned(k) ? static_cast<IntNTypeKind>(static_cast<int>(k) - static_cast<int>(IntNTypeKind::UnsignedStart)): k;
    }

    /**
     * checks if this type is bigger in terms of bit width, than the given type
     */
    bool greater_than_in_bits(IntNType* type) {
        return to_unsigned_kind(IntNKind()) > to_unsigned_kind(type->IntNKind());
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind() && equals(type->as_intn_type_unsafe());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

#endif

};

// ----------------------------
// --- Chemical Integer Types ---
// ----------------------------

class I8Type : public IntNType { public:
    inline constexpr I8Type() : IntNType(IntNTypeKind::I8) {}
};
class I16Type : public IntNType { public:
    inline constexpr I16Type() : IntNType(IntNTypeKind::I16) {}
};
class I32Type : public IntNType { public:
    inline constexpr I32Type() : IntNType(IntNTypeKind::I32) {}
};
class I64Type : public IntNType { public:
    inline constexpr I64Type() : IntNType(IntNTypeKind::I64) {}
};
class Int128Type : public IntNType { public:
    inline constexpr Int128Type() : IntNType(IntNTypeKind::Int128) {}
};

class U8Type : public IntNType { public:
    inline constexpr U8Type() : IntNType(IntNTypeKind::U8) {}
};
class U16Type : public IntNType { public:
    inline constexpr U16Type() : IntNType(IntNTypeKind::U16) {}
};
class U32Type : public IntNType { public:
    inline constexpr U32Type() : IntNType(IntNTypeKind::U32) {}
};
class U64Type : public IntNType { public:
    inline constexpr U64Type() : IntNType(IntNTypeKind::U64) {}
};
class UInt128Type : public IntNType { public:
    inline constexpr UInt128Type() : IntNType(IntNTypeKind::UInt128) {}
};

// ----------------------------
// --- C Like Integer Types ---
// ----------------------------

class CharType : public IntNType { public:
    inline constexpr CharType() : IntNType(IntNTypeKind::Char) {}
};
class ShortType : public IntNType { public:
    inline constexpr ShortType() : IntNType(IntNTypeKind::Short) {}
};
class IntType : public IntNType { public:
    inline constexpr IntType() : IntNType(IntNTypeKind::Int) {}
};
class LongType : public IntNType { public:
    inline constexpr LongType() : IntNType(IntNTypeKind::Long) {}
};
class LongLongType : public IntNType { public:
    inline constexpr LongLongType() : IntNType(IntNTypeKind::LongLong) {}
};

class UCharType : public IntNType { public:
    inline constexpr UCharType() : IntNType(IntNTypeKind::UChar) {}
};
class UShortType : public IntNType { public:
    inline constexpr UShortType() : IntNType(IntNTypeKind::UShort) {}
};
class UIntType : public IntNType { public:
    inline constexpr UIntType() : IntNType(IntNTypeKind::UInt) {}
};
class ULongType : public IntNType { public:
    inline constexpr ULongType() : IntNType(IntNTypeKind::ULong) {}
};
class ULongLongType : public IntNType { public:
    inline constexpr ULongLongType() : IntNType(IntNTypeKind::ULongLong) {}
};