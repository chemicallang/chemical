// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ASTAllocator.h"
#include "ast/types/IntNType.h"

/**
 * type builder helps build ast, in future it may serve
 * better by exposing methods to build certain ast things
 * currently it works as a caching system to cache certain types
 */
class TypeBuilder {
private:

    // c like integer types
    I8Type i8Type = I8Type();
    I16Type i16Type = I16Type();
    I32Type i32Type = I32Type();
    I64Type i64Type = I64Type();
    Int128Type int128Type = Int128Type();

    U8Type u8Type = U8Type();
    U16Type u16Type = U16Type();
    U32Type u32Type = U32Type();
    U64Type u64Type = U64Type();
    UInt128Type uInt128Type = UInt128Type();

    // c like integer types
    CharType charType = CharType();
    ShortType shortType = ShortType();
    IntType intType = IntType();
    LongType longType = LongType();
    LongLongType longLongType = LongLongType();

    // c like unsigned integer types
    UCharType uCharType = UCharType();
    UShortType uShortType = UShortType();
    UIntType uIntType = UIntType();
    ULongType uLongType = ULongType();
    ULongLongType uLongLongType = ULongLongType();

    // other types
    AnyType* anyType;
    BoolType* boolType;
    DoubleType* doubleType;
    Float128Type* float128Type;
    FloatType* floatType;
    LongDoubleType* longDoubleType;
    StringType* stringType;
    VoidType* voidType;
    NullPtrType* nullPtrType;
    PointerType* ptrToVoid;
    PointerType* ptrToAny;
    PointerType* constPtrToAny;
    ReferenceType* refToVoid;
    ReferenceType* refToAny;
    ExpressiveStringType* expr_str_type;

    // runtime types
    RuntimeType* runtimeAny;
    RuntimeType* runtimePtrToVoid;
    RuntimeType* runtimePtrToAny;
    RuntimeType* runtimeConstPtrToAny;

    // maybe runtime types
    MaybeRuntimeType* maybeRuntimeAny;
    MaybeRuntimeType* maybeRuntimePtrToVoid;

    // not exactly a type, but used once everywhere
    // when anything is unresolved, we link it with this declaration
    UnresolvedDecl* unresolvedDecl;

    NullValue* nullValue;

public:

    ASTAllocator& allocator;

    /**
     * void type is stored globally statically
     */
    static VoidType* getVoidTypeInstance();

    /**
     * constructor
     */
    inline TypeBuilder(
            ASTAllocator& allocator
    ) : allocator(allocator)
    {
        initialize();
    }

    /**
     * initialize method would reinitialize this on the allocator
     */
    void initialize();

    // -------------------------------
    // -- Chemical Integer Types --
    // -------------------------------

    inline I8Type* getI8Type() const noexcept { return const_cast<I8Type*>(&i8Type); }
    inline I16Type* getI16Type() const noexcept { return const_cast<I16Type*>(&i16Type); }
    inline I32Type* getI32Type() const noexcept { return const_cast<I32Type*>(&i32Type); }
    inline I64Type* getI64Type() const noexcept { return const_cast<I64Type*>(&i64Type); }
    inline Int128Type* getInt128Type() const noexcept { return const_cast<Int128Type*>(&int128Type); }

    inline U8Type* getU8Type() const noexcept { return const_cast<U8Type*>(&u8Type); }
    inline U16Type* getU16Type() const noexcept { return const_cast<U16Type*>(&u16Type); }
    inline U32Type* getU32Type() const noexcept { return const_cast<U32Type*>(&u32Type); }
    inline U64Type* getU64Type() const noexcept { return const_cast<U64Type*>(&u64Type); }
    inline UInt128Type* getUInt128Type() const noexcept { return const_cast<UInt128Type*>(&uInt128Type); }

    // -------------------------------
    // -- C Like Integer Types --
    // -------------------------------

    inline CharType* getCharType() const noexcept { return const_cast<CharType*>(&charType); }
    inline ShortType* getShortType() const noexcept { return const_cast<ShortType*>(&shortType); }
    inline IntType* getIntType() const noexcept { return const_cast<IntType*>(&intType); }
    inline LongType* getLongType() const noexcept { return const_cast<LongType*>(&longType); }
    inline LongLongType* getLongLongType() const noexcept { return const_cast<LongLongType*>(&longLongType); }

    inline UCharType* getUCharType() const noexcept { return const_cast<UCharType*>(&uCharType); }
    inline UShortType* getUShortType() const noexcept { return const_cast<UShortType*>(&uShortType); }
    inline UIntType* getUIntType() const noexcept { return const_cast<UIntType*>(&uIntType); }
    inline ULongType* getULongType() const noexcept { return const_cast<ULongType*>(&uLongType); }
    inline ULongLongType* getULongLongType() const noexcept { return const_cast<ULongLongType*>(&uLongLongType); }

    /**
     * method to get int n type for given bit width and signedness
     */
    IntNType* getIntNType(unsigned int bitWidth, bool isUnsigned) const noexcept {
        switch(bitWidth) {
            case 8:
                return isUnsigned ? (IntNType*) getU8Type() : (IntNType*) getI8Type();
            case 16:
                return isUnsigned ? (IntNType*) getU16Type() : (IntNType*) getI16Type();
            case 32:
            default:
                return isUnsigned ? (IntNType*) getU32Type() : (IntNType*) getI32Type();
            case 64:
                return isUnsigned ? (IntNType*) getU64Type() : (IntNType*) getI64Type();
            case 128:
                return isUnsigned ? (IntNType*) getInt128Type() : (IntNType*) getUInt128Type();
        }
    }

    /**
     * method to get int n type for given int n type kind
     */
    IntNType* getIntNType(IntNTypeKind kind) const noexcept {
        switch(kind) {
            case IntNTypeKind::Char: return getCharType();
            case IntNTypeKind::Short: return getShortType();
            case IntNTypeKind::Int: return getIntType();
            case IntNTypeKind::Long: return getLongType();
            case IntNTypeKind::LongLong: return getLongLongType();
            case IntNTypeKind::Int128: return getInt128Type();
            case IntNTypeKind::I8: return getI8Type();
            case IntNTypeKind::I16: return getI16Type();
            case IntNTypeKind::I32: return getI32Type();
            case IntNTypeKind::I64: return getI64Type();
            case IntNTypeKind::UChar: return getUCharType();
            case IntNTypeKind::UShort: return getUShortType();
            case IntNTypeKind::UInt: return getUIntType();
            case IntNTypeKind::ULong: return getULongType();
            case IntNTypeKind::ULongLong: return getULongLongType();
            case IntNTypeKind::UInt128: return getUInt128Type();
            case IntNTypeKind::U8: return getU8Type();
            case IntNTypeKind::U16: return getU16Type();
            case IntNTypeKind::U32: return getU32Type();
            case IntNTypeKind::U64: return getU64Type();
            default:
#ifdef DEBUG
                abort();
#endif
                return getIntType();
        }
    }

    // -------------------------------
    // -- Other Types --
    // -------------------------------

    /**
     * get AnyType
     */
    inline AnyType* getAnyType() const noexcept {
        return anyType;
    }

    /**
     * get BoolType
     */
    inline BoolType* getBoolType() const noexcept {
        return boolType;
    }

    /**
     * get DoubleType
     */
    inline DoubleType* getDoubleType() const noexcept {
        return doubleType;
    }

    /**
     * get Float128Type
     */
    inline Float128Type* getFloat128Type() const noexcept {
        return float128Type;
    }

    /**
     * get FloatType
     */
    inline FloatType* getFloatType() const noexcept {
        return floatType;
    }


    /**
     * get LongDoubleType
     */
    inline LongDoubleType* getLongDoubleType() const noexcept {
        return longDoubleType;
    }

    /**
     * get StringType
     */
    inline StringType* getStringType() const noexcept {
        return stringType;
    }

    /**
     * get VoidType
     */
    inline VoidType* getVoidType() const noexcept {
        return voidType;
    }

    /**
     * get NullPtrType
     */
    inline NullPtrType* getNullPtrType() const noexcept {
        return nullPtrType;
    }

    /**
     * get PointerType
     */
    inline PointerType* getPtrToVoid() const noexcept {
        return ptrToVoid;
    }

    /**
     * get PointerType
     */
    inline PointerType* getPtrToAny() const noexcept {
        return ptrToAny;
    }

    /**
     * get PointerType
     */
    inline PointerType* getConstPtrToAny() const noexcept {
        return constPtrToAny;
    }

    /**
     * get reference to any
     */
    inline ReferenceType* getRefToVoid() const noexcept {
        return refToVoid;
    }

    /**
     * get reference to any
     */
    inline ReferenceType* getRefToAny() const noexcept {
        return refToAny;
    }

    /**
     * get ExpressiveStringType
     */
    inline ExpressiveStringType* getExprStrType() const noexcept {
        return expr_str_type;
    }

    /**
     * get getRuntimeAnyType
     */
    inline RuntimeType* getRuntimeAnyType() const noexcept {
        return runtimeAny;
    }

    /**
     * get runtimePtrToVoid
     */
    inline RuntimeType* getRuntimePtrToVoid() const noexcept {
        return runtimePtrToVoid;
    }

    /**
     * get runtimePtrToVoid
     */
    inline RuntimeType* getRuntimePtrToAny() const noexcept {
        return runtimePtrToAny;
    }

    /**
     * get runtimePtrToVoid
     */
    inline RuntimeType* getRuntimeConstPtrToAny() const noexcept {
        return runtimeConstPtrToAny;
    }

    /**
     * get getRuntimeAnyType
     */
    inline MaybeRuntimeType* getMaybeRuntimeAnyType() const noexcept {
        return maybeRuntimeAny;
    }

    /**
     * get runtimePtrToVoid
     */
    inline MaybeRuntimeType* getMaybeRuntimePtrToVoid() const noexcept {
        return maybeRuntimePtrToVoid;
    }

    /**
     * get UnresolvedDecl
     */
    inline UnresolvedDecl* getUnresolvedDecl() const noexcept {
        return unresolvedDecl;
    }

    /**
     * get the null value
     */
    inline NullValue* getNullValue() const noexcept {
        return nullValue;
    }

};