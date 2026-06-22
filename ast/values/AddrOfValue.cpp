// Copyright (c) Chemical Language Foundation 2025.

#include "AddrOfValue.h"
#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/PointerType.h"
#include "ast/structures/FunctionParam.h"
#include "ReferenceOfValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/values/PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "StringValue.h"
#include "compiler/lab/TargetData.h"


void AddrOfValue::determine_type() {
    const auto valueType = value->getType();
    const auto can = valueType->canonical();
    getType()->type = can->kind() == BaseTypeKind::Reference ? can->as_reference_type_unsafe()->type : valueType;
}

uint64_t AddrOfValue::byte_size(TargetData& target) {
    return target.is64Bit ? 8 : 4;
}

Value* AddrOfValue::evaluated_value(InterpretScope &scope) {
    // Detect &arr[i] pattern — arr[i] is an AccessChain wrapping an IndexOperator,
    // or directly an IndexOperator (when arr is a plain variable reference).
    // Create a PointerValue into the array's contiguous memory so pointer arithmetic
    // spans the full array, not just one element.
    auto handleIndexOp = [&](IndexOperator* indexOp) -> Value* {
        auto arrEval = indexOp->parent_val->evaluated_value(scope);
        // If parent_val evaluates back to itself (a VariableIdentifier for module-level var),
        // extract the ArrayValue from the linked VarInitStmt
        if (arrEval && arrEval->val_kind() == ValueKind::Identifier) {
            auto ident = (VariableIdentifier*)arrEval;
            auto linkedNode = ident->linked_node();
            if (linkedNode && linkedNode->kind() == ASTNodeKind::VarInitStmt) {
                auto init = linkedNode->as_var_init_unsafe();
                if (init->value && init->value->val_kind() == ValueKind::ArrayValue) {
                    arrEval = init->value;
                }
            }
        }
        
        if (arrEval && arrEval->val_kind() == ValueKind::ArrayValue) {
            auto arrVal = (ArrayValue*)arrEval;
            // Ensure element type is determined (may be missing for AST-level ArrayValues)
            if (!arrVal->getType() || !arrVal->getType()->elem_type) {
                arrVal->determine_type(scope.allocator);
            }
            // Ensure element storage is initialized (lazy allocation in evaluated_value)
            arrVal->evaluated_value(scope);

            // Try contiguousData first (primitive element types)
            auto cd = arrVal->contiguousData;
            auto cs = arrVal->contiguousSize;
            if (cd && cs > 0) {
                const auto ptrType = getType();
                const auto pointeeType = ptrType->type;
                const auto elemSize = pointeeType->byte_size(scope.global->target_data);
                if (elemSize > 0) {
                    auto idxEval = indexOp->idx->evaluated_value(scope);
                    auto idxOpt = idxEval->get_number();
                    if (idxOpt.has_value()) {
                        auto idx = idxOpt.value();
                        auto offset = idx * elemSize;
                        if (offset < cs) {
                            auto ahead = cs - offset;
                            return new (scope.allocate<PointerValue>()) PointerValue(
                                (char*)cd + offset, pointeeType,
                                offset, ahead, encoded_location()
                            );
                        }
                    }
                }
            }

            // Fallback: struct elements stored in the values vector
            auto idxEval = indexOp->idx->evaluated_value(scope);
            auto idxOpt = idxEval->get_number();
            if (idxOpt.has_value()) {
                auto idx = idxOpt.value();
                if (idx < arrVal->values.size()) {
                    auto elemVal = arrVal->values[idx];
                    if (elemVal && elemVal->val_kind() == ValueKind::StructValue) {
                        const auto ptrType = getType();
                        const auto pointeeType = ptrType->type;
                        const auto elemSize = pointeeType->byte_size(scope.global->target_data);
                        return new (scope.allocate<PointerValue>()) PointerValue(
                            (StructValue*)elemVal, pointeeType, 0, elemSize, encoded_location()
                        );
                    }
                }
            }
        }
        return nullptr;
    };

    if (value->val_kind() == ValueKind::AccessChain) {
        auto chain = (AccessChain*)value;
        if (!chain->values.empty()) {
            auto lastVal = chain->values.back();
            if (lastVal->val_kind() == ValueKind::IndexOperator) {
                auto result = handleIndexOp((IndexOperator*)lastVal);
                if (result) return result;
            }
        }
    } else if (value->val_kind() == ValueKind::IndexOperator) {
        auto result = handleIndexOp((IndexOperator*)value);
        if (result) return result;
    }
    
    const auto inner = value->evaluated_value(scope);
    if(!inner) return nullptr;
    const auto ptrType = getType();
    const auto pointeeType = ptrType->type;
    const auto byteSize = pointeeType->byte_size(scope.global->target_data);
    switch(inner->val_kind()) {
        case ValueKind::IntN: {
            auto intVal = (IntNumValue*) inner;
            // Use pointeeType (not ptrType) so pointer arithmetic uses element size, not pointer size
            return new (scope.allocate<PointerValue>()) PointerValue(
                &intVal->value, pointeeType, 0, byteSize, encoded_location()
            );
        }
        case ValueKind::String: {
            auto strVal = inner->as_string_unsafe();
            // Use pointeeType so pointer arithmetic uses element size (1 byte for char)
            return new (scope.allocate<PointerValue>()) PointerValue(
                scope, strVal, pointeeType
            );
        }
        case ValueKind::StructValue: {
            auto structVal = (StructValue*) inner;
            return new (scope.allocate<PointerValue>()) PointerValue(
                structVal, pointeeType, 0, byteSize, encoded_location()
            );
        }
        case ValueKind::PointerValue: {
            // Taking address of a pointer: create a new PointerValue pointing to the pointer's data
            auto ptrVal = (PointerValue*) inner;
            return new (scope.allocate<PointerValue>()) PointerValue(
                ptrVal->data, pointeeType, 0, ptrVal->ahead, encoded_location()
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
        default:
            // Fall back to base class behavior: return this (the AddrOfValue AST node)
            // This allows unsupported types to be handled by codegen rather than crashing here
            return this;
    }
}

void ReferenceOfValue::determine_type() {
    const auto valueType = value->getType();
    getType()->type = valueType;
}

uint64_t ReferenceOfValue::byte_size(TargetData& target) {
    return target.is64Bit ? 8 : 4;
}