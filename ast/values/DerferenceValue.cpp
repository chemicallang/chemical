// Copyright (c) Chemical Language Foundation 2025.

#include "DereferenceValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/BaseType.h"
#include "ast/values/StringValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/StructValue.h"
#include "ast/base/InterpretScope.h"
#include <iostream>

bool DereferenceValue::determine_type(const TypeBuilder& typeBuilder) {
    const auto type = value->getType();
    switch(type->kind()) {
        case BaseTypeKind::Pointer:
            setType(type->as_pointer_type_unsafe()->type);
            return true;
        case BaseTypeKind::Reference:
            setType(type->as_reference_type_unsafe()->type);
            return true;
        case BaseTypeKind::String:
            setType(typeBuilder.getCharType());
            return true;
        default:
            setType((BaseType*) typeBuilder.getVoidType());
            return false;
    }
}

Value* DereferenceValue::child(InterpretScope& scope, const chem::string_view& name) {
    // `(*ptr).member` — resolve the member inside the pointee, not in a copy of
    // it. Evaluating the inner value yields the pointer itself (a PointerValue),
    // whose child() lookup finds the member in the struct it points at, so
    // writes through the chain reach the pointee.
    const auto eval = value->evaluated_value(scope);
    if(eval == nullptr) {
        return nullptr;
    }
    return eval->child(scope, name);
}

DereferenceValue *DereferenceValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<DereferenceValue>()) DereferenceValue(
            value->copy(allocator),
            getType(),
            encoded_location()
    );
}