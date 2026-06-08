// Copyright (c) Chemical Language Foundation 2025.

#include "DereferenceValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/StringValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/base/InterpretScope.h"
#include <iostream>

bool DereferenceValue::determine_type(TypeBuilder& typeBuilder) {
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

Value* DereferenceValue::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    const auto k = eval->val_kind();
    switch(k) {
        case ValueKind::String:{
            const auto val = eval->as_string_unsafe();
            return new (scope.allocate<IntNumValue>()) IntNumValue(val->value[0], scope.global->typeBuilder.getCharType(), encoded_location());
        }
        case ValueKind::PointerValue: {
            const auto val = (PointerValue*) eval;
            return val->deref(scope, encoded_location(), this);
        }
        default:
            scope.error("couldn't dereference value in comptime", this);
            return eval;
    }
}

DereferenceValue *DereferenceValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<DereferenceValue>()) DereferenceValue(
            value->copy(allocator),
            getType(),
            encoded_location()
    );
}