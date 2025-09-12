// Copyright (c) Chemical Language Foundation 2025.

#include "TypeBuilder.h"
#include "ast/types/AnyType.h"
#include "ast/types/BoolType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/FloatType.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/StringType.h"
#include "ast/types/VoidType.h"
#include "ast/types/NullPtrType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ExpressiveStringType.h"

void TypeBuilder::initialize() {

    anyType = new (allocator.allocate<AnyType>()) AnyType();
    boolType = new (allocator.allocate<BoolType>()) BoolType();
    doubleType = new (allocator.allocate<DoubleType>()) DoubleType();
    float128Type = new (allocator.allocate<Float128Type>()) Float128Type();
    floatType = new (allocator.allocate<FloatType>()) FloatType();
    longDoubleType = new (allocator.allocate<LongDoubleType>()) LongDoubleType();
    stringType = new (allocator.allocate<StringType>()) StringType();
    voidType = new (allocator.allocate<VoidType>()) VoidType();
    nullPtrType = new (allocator.allocate<NullPtrType>()) NullPtrType();
    ptrToVoid = new (allocator.allocate<PointerType>()) PointerType(voidType, true);
    expr_str_type = new (allocator.allocate<ExpressiveStringType>()) ExpressiveStringType();

}