// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ASTAllocator.h"

/**
 * type builder helps build ast, in future it may serve
 * better by exposing methods to build certain ast things
 * currently it works as a caching system to cache certain types
 */
class TypeBuilder {
private:

    AnyType* anyType;
    BigIntType* bigIntType;
    BoolType* boolType;
    CharType* charType;
    DoubleType* doubleType;
    Float128Type* float128Type;
    FloatType* floatType;
    Int128Type* int128Type;
    IntType* intType;
    LongDoubleType* longDoubleType;
    LongType* longType;
    ShortType* shortType;
    StringType* stringType;
    UBigIntType* uBigIntType;
    UCharType* uCharType;
    UInt128Type* uInt128Type;
    UIntType* uIntType;
    ULongType* uLongType;
    UShortType* uShortType;
    VoidType* voidType;

public:

    ASTAllocator& allocator;

    /**
     * constructor
     */
    inline TypeBuilder(ASTAllocator& allocator) : allocator(allocator) {
        initialize();
    }

    /**
     * initialize method would reinitialize this on the allocator
     */
    void initialize();

    /**
     * get AnyType
     */
    AnyType* getAnyType() {
        return anyType;
    }
    /**
     * get BigIntType
     */
    BigIntType* getBigIntType() {
        return bigIntType;
    }

    /**
     * get BoolType
     */
    BoolType* getBoolType() {
        return boolType;
    }

    /**
     * get CharType
     */
    CharType* getCharType() {
        return charType;
    }

    /**
     * get DoubleType
     */
    DoubleType* getDoubleType() {
        return doubleType;
    }

    /**
     * get Float128Type
     */
    Float128Type* getFloat128Type() {
        return float128Type;
    }

    /**
     * get FloatType
     */
    FloatType* getFloatType() {
        return floatType;
    }

    /**
     * get Int128Type
     */
    Int128Type* getInt128Type() {
        return int128Type;
    }

    /**
     * get IntType
     */
    IntType* getIntType() {
        return intType;
    }

    /**
     * get LongDoubleType
     */
    LongDoubleType* getLongDoubleType() {
        return longDoubleType;
    }

    /**
     * get LongType
     */
    LongType* getLongType() {
        return longType;
    }

    /**
     * get ShortType
     */
    ShortType* getShortType() {
        return shortType;
    }

    /**
     * get StringType
     */
    StringType* getStringType() {
        return stringType;
    }

    /**
     * get UBigIntType
     */
    UBigIntType* getUBigIntType() {
        return uBigIntType;
    }

    /**
     * get UCharType
     */
    UCharType* getUCharType() {
        return uCharType;
    }

    /**
     * get UInt128Type
     */
    UInt128Type* getUInt128Type() {
        return uInt128Type;
    }

    /**
     * get UIntType
     */
    UIntType* getUIntType() {
        return uIntType;
    }

    /**
     * get ULongType
     */
    ULongType* getULongType() {
        return uLongType;
    }

    /**
     * get UShortType
     */
    UShortType* getUShortType() {
        return uShortType;
    }

    /**
     * get VoidType
     */
    VoidType* getVoidType() {
        return voidType;
    }

};