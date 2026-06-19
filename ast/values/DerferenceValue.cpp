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
    if(!eval) return nullptr;
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
            // For non-pointer values, the DereferenceValue represents an implicit
            // reference dereference (e.g., accessing a local variable).
            // The value IS already the dereferenced value, return it directly.
            return eval;
    }
}

void DereferenceValue::set_value(InterpretScope& scope, Value* rawValue, Operation op, SourceLocation location) {
    // Evaluate the inner expression first
    const auto ptrEval = value->evaluated_value(scope);
    if(ptrEval->val_kind() == ValueKind::PointerValue) {
        // This is a real pointer dereference (e.g., *ptr = value)
        auto ptrVal = (PointerValue*) ptrEval;
        const auto pointeeType = getType();
        const auto byteSize = pointeeType->byte_size(scope.global->target_data);
        if(byteSize > ptrVal->ahead) {
            scope.error("cannot dereference pointer while type size is larger than bytes available", this);
            return;
        }
        // Evaluate the new value
        const auto newVal = rawValue->evaluated_value(scope);
        if(!newVal) return;
        // Write the new value's data to the pointer location
        const auto num = newVal->get_number();
        if(num.has_value()) {
            switch(byteSize) {
                case 1: *((char*) ptrVal->data) = (char) num.value(); break;
                case 2: *((short*) ptrVal->data) = (short) num.value(); break;
                case 4: *((int*) ptrVal->data) = (int) num.value(); break;
                case 8:
                default: *((uint64_t*) ptrVal->data) = num.value(); break;
            }
        } else if(newVal->val_kind() == ValueKind::String) {
            auto strVal = newVal->as_string_unsafe();
            memcpy(ptrVal->data, strVal->value.data(), std::min(strVal->value.size(), (size_t)byteSize));
        } else {
            scope.error("cannot assign value type through pointer dereference in interpret", this);
        }
    } else {
        // Non-pointer inner: implicit reference deref (e.g., local variable access)
        // Delegate to the inner value's set_value to handle assignment through scope
        value->set_value(scope, rawValue, op, location);
    }
}

DereferenceValue *DereferenceValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<DereferenceValue>()) DereferenceValue(
            value->copy(allocator),
            getType(),
            encoded_location()
    );
}