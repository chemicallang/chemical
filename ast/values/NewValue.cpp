// Copyright (c) Chemical Language Foundation 2025.

#include "NewValue.h"
#include "PlacementNewValue.h"
#include "NewTypedValue.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/PointerType.h"
#include "ast/types/IntNType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/structures/VariantDefinition.h"
#include <cstring>

Value* NewValue::evaluated_value(InterpretScope& scope) {
    auto inner = value->evaluated_value(scope);
    if(!inner) return scope.getNullValue();
    auto innerType = inner->getType();
    if(!innerType) return scope.getNullValue();
    
    // Evaluate inner, wrap in PointerValue pointing to the inner value's data.
    // For structs, PointerValue stores the StructValue* directly in data.
    // For primitives, PointerValue stores a pointer to the value field.
    if(inner->val_kind() == ValueKind::StructValue) {
        auto byteSize = innerType->byte_size(scope.global->target_data);
        return new (scope.allocate<PointerValue>()) PointerValue(
            inner, innerType, 0, byteSize, encoded_location()
        );
    } else if(inner->val_kind() == ValueKind::IntN) {
        auto intVal = (IntNumValue*)inner;
        return new (scope.allocate<PointerValue>()) PointerValue(
            &intVal->value, innerType, 0, sizeof(uint64_t), encoded_location()
        );
    } else if(inner->val_kind() == ValueKind::Bool) {
        auto boolVal = (BoolValue*)inner;
        return new (scope.allocate<PointerValue>()) PointerValue(
            &boolVal->value, innerType, 0, 1, encoded_location()
        );
    } else if(inner->val_kind() == ValueKind::Float) {
        auto floatVal = (FloatValue*)inner;
        return new (scope.allocate<PointerValue>()) PointerValue(
            &floatVal->value, innerType, 0, sizeof(float), encoded_location()
        );
    } else if(inner->val_kind() == ValueKind::Double) {
        auto doubleVal = (DoubleValue*)inner;
        return new (scope.allocate<PointerValue>()) PointerValue(
            &doubleVal->value, innerType, 0, sizeof(double), encoded_location()
        );
    } else {
        auto byteSize = innerType->byte_size(scope.global->target_data);
        return new (scope.allocate<PointerValue>()) PointerValue(
            inner, innerType, 0, byteSize, encoded_location()
        );
    }
}

Value* NewTypedValue::evaluated_value(InterpretScope& scope) {
    auto ptrType = (PointerType*)getType();
    auto pointeeType = ptrType ? ptrType->type : nullptr;
    if(!pointeeType) return scope.getNullValue();
    auto canonical = pointeeType->canonical();
    
    switch(canonical->kind()) {
        case BaseTypeKind::IntN: {
            auto val = new (scope.allocate<IntNumValue>()) IntNumValue(
                0, (IntNType*)canonical, encoded_location()
            );
            return new (scope.allocate<PointerValue>()) PointerValue(
                &val->value, canonical, 0, sizeof(uint64_t), encoded_location()
            );
        }
        case BaseTypeKind::Bool: {
            auto val = new (scope.allocate<BoolValue>()) BoolValue(
                false, scope.global->typeBuilder.getBoolType(), encoded_location()
            );
            return new (scope.allocate<PointerValue>()) PointerValue(
                &val->value, canonical, 0, 1, encoded_location()
            );
        }
        case BaseTypeKind::Float: {
            auto val = new (scope.allocate<FloatValue>()) FloatValue(
                0.0f, (FloatType*)canonical, encoded_location()
            );
            return new (scope.allocate<PointerValue>()) PointerValue(
                &val->value, canonical, 0, sizeof(float), encoded_location()
            );
        }
        case BaseTypeKind::Double: {
            auto val = new (scope.allocate<DoubleValue>()) DoubleValue(
                0.0, (DoubleType*)canonical, encoded_location()
            );
            return new (scope.allocate<PointerValue>()) PointerValue(
                &val->value, canonical, 0, sizeof(double), encoded_location()
            );
        }
        case BaseTypeKind::Pointer: {
            // Pointer to pointer (e.g. `new *int`): allocate raw untracked memory
            // (not tracked in ptr_storage) so that dereference+assignment through the
            // pointer writes to raw memory instead of corrupting a tracked AST object's
            // vtable. The initial value is zero (null pointer).
            auto storage = scope.allocator.allocate_released_size(sizeof(void*), alignof(void*));
            std::memset(storage, 0, sizeof(void*));
            return new (scope.allocate<PointerValue>()) PointerValue(
                storage, canonical, 0, sizeof(void*), encoded_location()
            );
        }
        case BaseTypeKind::Linked: {
            auto node = canonical->get_direct_linked_canonical_node();
            if(node && node->kind() == ASTNodeKind::StructDecl) {
                auto structDef = (StructDefinition*)node;
                auto structVal = new (scope.allocate<StructValue>()) StructValue(
                    TypeLoc(canonical->copy(scope.allocator), encoded_location()),
                    structDef,
                    (VariablesContainerBase*)structDef,
                    encoded_location()
                );
                for(const auto field : structDef->variables()) {
                    auto defVal = field->default_value();
                    if(defVal) {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer{field->name, defVal->scope_value(scope)}
                        );
                    } else {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer{field->name, scope.getNullValue()}
                        );
                    }
                }
                return new (scope.allocate<PointerValue>()) PointerValue(
                    structVal, canonical, 0,
                    canonical->byte_size(scope.global->target_data),
                    encoded_location()
                );
            }
            if(node && node->kind() == ASTNodeKind::VariantDecl) {
                auto varDef = (VariantDefinition*)node;
                auto structVal = new (scope.allocate<StructValue>()) StructValue(
                    TypeLoc(canonical->copy(scope.allocator), encoded_location()),
                    varDef,
                    (VariablesContainerBase*)varDef,
                    encoded_location()
                );
                return new (scope.allocate<PointerValue>()) PointerValue(
                    structVal, canonical, 0,
                    canonical->byte_size(scope.global->target_data),
                    encoded_location()
                );
            }
            return scope.getNullValue();
        }
        default:
            return scope.getNullValue();
    }
}

Value* PlacementNewValue::evaluated_value(InterpretScope& scope) {
    // Evaluate the pointer
    auto ptrEval = pointer->evaluated_value(scope);
    if(!ptrEval || ptrEval->val_kind() != ValueKind::PointerValue) {
        scope.error("placement new requires a pointer", this);
        return scope.getNullValue();
    }
    auto ptrVal = (PointerValue*)ptrEval;
    // Evaluate the value to place
    auto valEval = value->evaluated_value(scope);
    if(!valEval) return scope.getNullValue();
    // Write the value to the location pointed to by ptr
    if(valEval->val_kind() == ValueKind::StructValue) {
        auto srcStruct = valEval->as_struct_value_unsafe();
        auto dstStruct = (StructValue*)ptrVal->data;
        if(dstStruct) {
            for(auto& [name, member] : srcStruct->values) {
                dstStruct->set_child_value(scope, name, member.value, Operation::Assignment);
            }
        }
    } else if(valEval->val_kind() == ValueKind::FunctionCall) {
        // Placement new with a function call (e.g. @make constructor)
        auto callResult = valEval->evaluated_value(scope);
        if(callResult && callResult->val_kind() == ValueKind::StructValue) {
            auto srcStruct = callResult->as_struct_value_unsafe();
            auto dstStruct = (StructValue*)ptrVal->data;
            if(dstStruct) {
                for(auto& [name, member] : srcStruct->values) {
                    dstStruct->set_child_value(scope, name, member.value, Operation::Assignment);
                }
            }
        }
    } else {
        const auto num = valEval->get_number();
        if(num.has_value()) {
            auto byteSize = valEval->getType()->byte_size(scope.global->target_data);
            switch(byteSize) {
                case 1: *((char*)ptrVal->data) = (char)num.value(); break;
                case 2: *((short*)ptrVal->data) = (short)num.value(); break;
                case 4: *((int*)ptrVal->data) = (int)num.value(); break;
                case 8:
                default: *((uint64_t*)ptrVal->data) = num.value(); break;
            }
        }
    }
    return ptrVal;
}
