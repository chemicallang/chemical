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
#include "ast/types/RuntimeType.h"
#include "ast/types/MaybeRuntimeType.h"
#include "ast/statements/UnresolvedDecl.h"
#include "ast/values/NullValue.h"
#include "ast/types/ExpressiveStringType.h"

static const VoidType voidTypeInstance;

VoidType* TypeBuilder::getVoidTypeInstance() {
    return const_cast<VoidType*>(&voidTypeInstance);
}

void TypeBuilder::initialize() {

    anyType = new (allocator.allocate<AnyType>()) AnyType();
    boolType = new (allocator.allocate<BoolType>()) BoolType();
    doubleType = new (allocator.allocate<DoubleType>()) DoubleType();
    float128Type = new (allocator.allocate<Float128Type>()) Float128Type();
    floatType = new (allocator.allocate<FloatType>()) FloatType();
    longDoubleType = new (allocator.allocate<LongDoubleType>()) LongDoubleType();
    stringType = new (allocator.allocate<StringType>()) StringType();
    voidType = const_cast<VoidType*>(&voidTypeInstance);
    nullPtrType = new (allocator.allocate<NullPtrType>()) NullPtrType();
    ptrToVoid = new (allocator.allocate<PointerType>()) PointerType(voidType, true);
    ptrToAny = new (allocator.allocate<PointerType>()) PointerType(anyType, true);
    constPtrToAny = new (allocator.allocate<PointerType>()) PointerType(anyType, false);
    expr_str_type = new (allocator.allocate<ExpressiveStringType>()) ExpressiveStringType();

    // runtime types
    runtimeAny = new (allocator.allocate<RuntimeType>()) RuntimeType(anyType);
    runtimePtrToVoid = new (allocator.allocate<RuntimeType>()) RuntimeType(ptrToVoid);
    runtimeConstPtrToAny = new (allocator.allocate<RuntimeType>()) RuntimeType(constPtrToAny);
    runtimePtrToAny = new (allocator.allocate<RuntimeType>()) RuntimeType(ptrToVoid);

    // maybe runtime types
    maybeRuntimeAny = new (allocator.allocate<MaybeRuntimeType>()) MaybeRuntimeType(anyType);
    maybeRuntimePtrToVoid = new (allocator.allocate<MaybeRuntimeType>()) MaybeRuntimeType(ptrToVoid);

    // global declarations
    unresolvedDecl = new (allocator.allocate<UnresolvedDecl>()) UnresolvedDecl(nullptr, voidType, ZERO_LOC);

    // values
    nullValue = new (allocator.allocate<NullValue>()) NullValue(nullPtrType, ZERO_LOC);

}