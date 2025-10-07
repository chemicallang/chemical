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
    ExpressiveStringType* expr_str_type;

    // runtime types
    RuntimeType* runtimeAny;
    RuntimeType* runtimePtrToVoid;

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

    inline I8Type* getI8Type() noexcept { return &i8Type; }
    inline I16Type* getI16Type() noexcept { return &i16Type; }
    inline I32Type* getI32Type() noexcept { return &i32Type; }
    inline I64Type* getI64Type() noexcept { return &i64Type; }
    inline Int128Type* getInt128Type() noexcept { return &int128Type; }

    inline U8Type* getU8Type() noexcept { return &u8Type; }
    inline U16Type* getU16Type() noexcept { return &u16Type; }
    inline U32Type* getU32Type() noexcept { return &u32Type; }
    inline U64Type* getU64Type() noexcept { return &u64Type; }
    inline UInt128Type* getUInt128Type() noexcept { return &uInt128Type; }

    // -------------------------------
    // -- C Like Integer Types --
    // -------------------------------

    inline CharType* getCharType() noexcept { return &charType; }
    inline ShortType* getShortType() noexcept { return &shortType; }
    inline IntType* getIntType() noexcept { return &intType; }
    inline LongType* getLongType() noexcept { return &longType; }
    inline LongLongType* getLongLongType() noexcept { return &longLongType; }

    inline UCharType* getUCharType() noexcept { return &uCharType; }
    inline UShortType* getUShortType() noexcept { return &uShortType; }
    inline UIntType* getUIntType() noexcept { return &uIntType; }
    inline ULongType* getULongType() noexcept { return &uLongType; }
    inline ULongLongType* getULongLongType() noexcept { return &uLongLongType; }

    /**
     * method to get int n type for given bit width and signedness
     */
    IntNType* getIntNType(unsigned int bitWidth, bool isUnsigned) noexcept {
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
    IntNType* getIntNType(IntNTypeKind kind) noexcept {
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
    inline BoolType* getBoolType() {
        return boolType;
    }

    /**
     * get DoubleType
     */
    inline DoubleType* getDoubleType() {
        return doubleType;
    }

    /**
     * get Float128Type
     */
    inline Float128Type* getFloat128Type() {
        return float128Type;
    }

    /**
     * get FloatType
     */
    inline FloatType* getFloatType() {
        return floatType;
    }


    /**
     * get LongDoubleType
     */
    inline LongDoubleType* getLongDoubleType() {
        return longDoubleType;
    }

    /**
     * get StringType
     */
    inline StringType* getStringType() {
        return stringType;
    }

    /**
     * get VoidType
     */
    inline VoidType* getVoidType() {
        return voidType;
    }

    /**
     * get NullPtrType
     */
    inline NullPtrType* getNullPtrType() {
        return nullPtrType;
    }

    /**
     * get PointerType
     */
    inline PointerType* getPtrToVoid() {
        return ptrToVoid;
    }

    /**
     * get PointerType
     */
    inline PointerType* getPtrToAny() {
        return ptrToAny;
    }

    /**
     * get PointerType
     */
    inline PointerType* getConstPtrToAny() {
        return constPtrToAny;
    }

    /**
     * get ExpressiveStringType
     */
    inline ExpressiveStringType* getExprStrType() {
        return expr_str_type;
    }

    /**
     * get getRuntimeAnyType
     */
    inline RuntimeType* getRuntimeAnyType() {
        return runtimeAny;
    }

    /**
     * get runtimePtrToVoid
     */
    inline RuntimeType* getRuntimePtrToVoid() {
        return runtimePtrToVoid;
    }

    /**
     * get getRuntimeAnyType
     */
    inline MaybeRuntimeType* getMaybeRuntimeAnyType() {
        return maybeRuntimeAny;
    }

    /**
     * get runtimePtrToVoid
     */
    inline MaybeRuntimeType* getMaybeRuntimePtrToVoid() {
        return maybeRuntimePtrToVoid;
    }

    /**
     * get UnresolvedDecl
     */
    inline UnresolvedDecl* getUnresolvedDecl() {
        return unresolvedDecl;
    }

    /**
     * get the null value
     */
    inline NullValue* getNullValue() {
        return nullValue;
    }

};