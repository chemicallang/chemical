// Copyright (c) Chemical Language Foundation 2026.

//
// Created by wakaztahir on 6/4/26.
//

#include "ReferenceOfValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StringValue.h"
#include "ast/types/PointerType.h"

Value* ReferenceOfValue::evaluated_value(InterpretScope& scope) {
    // Evaluate the inner expression to get its value
    const auto inner = value->evaluated_value(scope);
    if(!inner) return nullptr;
    
    // Get the reference type and its underlying type
    const auto refType = getType();
    const auto pointeeType = refType ? refType->as_reference_type_unsafe()->type : nullptr;
    
    // Create a PointerValue pointing to the inner value's data
    // This allows DereferenceValue (*) to read/write through the reference
    switch(inner->val_kind()) {
        case ValueKind::IntN: {
            auto intVal = (IntNumValue*) inner;
            const auto byteSize = pointeeType ? pointeeType->byte_size(scope.global->target_data) : 8;
            return new (scope.allocate<PointerValue>()) PointerValue(
                &intVal->value, pointeeType, 0, byteSize, encoded_location()
            );
        }
        case ValueKind::Bool: {
            auto boolVal = (BoolValue*) inner;
            return new (scope.allocate<PointerValue>()) PointerValue(
                &boolVal->value, pointeeType, 0, 1, encoded_location()
            );
        }
        case ValueKind::Float: {
            auto floatVal = (FloatValue*) inner;
            return new (scope.allocate<PointerValue>()) PointerValue(
                &floatVal->value, pointeeType, 0, sizeof(float), encoded_location()
            );
        }
        case ValueKind::Double: {
            auto doubleVal = (DoubleValue*) inner;
            return new (scope.allocate<PointerValue>()) PointerValue(
                &doubleVal->value, pointeeType, 0, sizeof(double), encoded_location()
            );
        }
        case ValueKind::String: {
            auto strVal = inner->as_string_unsafe();
            return new (scope.allocate<PointerValue>()) PointerValue(
                scope, strVal, pointeeType
            );
        }
        default:
            // For unsupported types, fall back to returning inner directly
            return inner;
    }
}
