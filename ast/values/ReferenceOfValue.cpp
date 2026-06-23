// Copyright (c) Chemical Language Foundation 2026.

//
// Created by wakaztahir on 6/4/26.
//

#include "ReferenceOfValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/AccessChain.h"
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
    // &mut arr[i] pattern: arr[i] is an AccessChain wrapping an IndexOperator.
    // Get the ArrayValue from the IndexOperator's parent_val directly.
    if (value->val_kind() == ValueKind::AccessChain) {
        auto chain = (AccessChain*)value;
        if (!chain->values.empty()) {
            auto lastVal = chain->values.back();
            if (lastVal->val_kind() == ValueKind::IndexOperator) {
                auto indexOp = (IndexOperator*)lastVal;
                // parent_val is the array variable — evaluate it to get the ArrayValue
                auto arrEval = indexOp->parent_val->evaluated_value(scope);
                if (arrEval && arrEval->val_kind() == ValueKind::ArrayValue) {
                    auto arrVal = (ArrayValue*)arrEval;
                    if (arrVal->contiguousData && arrVal->contiguousSize > 0) {
                        const auto refType = getType();
                        const auto pointeeType = refType ? refType->as_reference_type_unsafe()->type : nullptr;
                        if (pointeeType) {
                            const auto elemSize = pointeeType->byte_size(scope.global->target_data);
                            if (elemSize > 0) {
                                auto idxEval = indexOp->idx->evaluated_value(scope);
                                auto idxOpt = idxEval->get_number();
                                if (idxOpt.has_value()) {
                                    auto idx = idxOpt.value();
                                    auto offset = idx * elemSize;
                                    if (offset < arrVal->contiguousSize) {
                                        auto ahead = arrVal->contiguousSize - offset;
                                        return new (scope.allocate<PointerValue>()) PointerValue(
                                            (char*)arrVal->contiguousData + offset, pointeeType,
                                            offset, ahead, encoded_location()
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
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
        case ValueKind::StructValue: {
            auto structVal = (StructValue*) inner;
            const auto byteSize = pointeeType ? pointeeType->byte_size(scope.global->target_data) : 0;
            return new (scope.allocate<PointerValue>()) PointerValue(
                structVal, pointeeType, 0, byteSize, encoded_location()
            );
        }
        default:
            // For unsupported types, fall back to returning inner directly
            return inner;
    }
}
