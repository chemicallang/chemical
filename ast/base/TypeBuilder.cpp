// Copyright (c) Chemical Language Foundation 2025.

#include "TypeBuilder.h"
#include "ast/types/AnyType.h"
#include "ast/types/BigIntType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/FloatType.h"
#include "ast/types/Int128Type.h"
#include "ast/types/IntType.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/LongType.h"
#include "ast/types/ShortType.h"
#include "ast/types/StringType.h"
#include "ast/types/UBigIntType.h"
#include "ast/types/UCharType.h"
#include "ast/types/UInt128Type.h"
#include "ast/types/UIntType.h"
#include "ast/types/ULongType.h"
#include "ast/types/UShortType.h"
#include "ast/types/VoidType.h"

void TypeBuilder::initialize() {

    anyType = new (allocator.allocate<AnyType>()) AnyType();
    bigIntType = new (allocator.allocate<BigIntType>()) BigIntType();
    boolType = new (allocator.allocate<BoolType>()) BoolType();
    charType = new (allocator.allocate<CharType>()) CharType();
    doubleType = new (allocator.allocate<DoubleType>()) DoubleType();
    float128Type = new (allocator.allocate<Float128Type>()) Float128Type();
    floatType = new (allocator.allocate<FloatType>()) FloatType();
    int128Type = new (allocator.allocate<Int128Type>()) Int128Type();
    intType = new (allocator.allocate<IntType>()) IntType();
    longDoubleType = new (allocator.allocate<LongDoubleType>()) LongDoubleType();
    longType = new (allocator.allocate<LongType>()) LongType();
    shortType = new (allocator.allocate<ShortType>()) ShortType();
    stringType = new (allocator.allocate<StringType>()) StringType();
    uBigIntType = new (allocator.allocate<UBigIntType>()) UBigIntType();
    uCharType = new (allocator.allocate<UCharType>()) UCharType();
    uInt128Type = new (allocator.allocate<UInt128Type>()) UInt128Type();
    uIntType = new (allocator.allocate<UIntType>()) UIntType();
    uLongType = new (allocator.allocate<ULongType>()) ULongType();
    uShortType = new (allocator.allocate<UShortType>()) UShortType();
    voidType = new (allocator.allocate<VoidType>()) VoidType();

}