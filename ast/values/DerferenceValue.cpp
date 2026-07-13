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
        default: {
            // Try evaluating further (handles AddrOfValue→PointerValue, etc.)
            const auto further = eval->evaluated_value(scope);
            if(further && further != eval) {
                if(further->val_kind() == ValueKind::PointerValue) {
                    return ((PointerValue*)further)->deref(scope, encoded_location(), this);
                }
                return further;
            }
            return eval;
        }
    }
}

void DereferenceValue::set_value(InterpretScope& scope, Value* rawValue, Operation op, SourceLocation location) {
    // Evaluate the inner expression first
    const auto ptrEval = value->evaluated_value(scope);
    if(!ptrEval) {
        scope.error("cannot assign through dereference: inner value could not be evaluated", this);
        return;
    }
    if(ptrEval->val_kind() == ValueKind::PointerValue) {
        // This is a real pointer dereference (e.g., *ptr = value)
        auto ptrVal = (PointerValue*) ptrEval;
        const auto pointeeType = getType();
        const auto byteSize = pointeeType->byte_size(scope.global->target_data);
        if(byteSize > ptrVal->ahead) {
            // bounds exceeded at runtime — just skip the write (native codegen doesn't bounds-check)
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
        } else if(newVal->val_kind() == ValueKind::StructValue) {
            auto srcStruct = (StructValue*) newVal;
            // Check if the pointee type supports struct semantics
            if(pointeeType->kind() == BaseTypeKind::Linked || pointeeType->kind() == BaseTypeKind::Pointer) {
                auto targetStruct = (StructValue*) ptrVal->data;
                if(targetStruct) {
                    // Copy all member values from source to destination
                    for(auto& [name, member] : srcStruct->values) {
                        targetStruct->set_child_value(scope, name, member.value, Operation::Assignment);
                    }
                } else {
                    // Write the struct pointer directly into the memory
                    *((void**)ptrVal->data) = (void*)srcStruct;
                }
            } else if(byteSize > 0 && ptrVal->data) {
                // Write the struct value's data into the memory location as raw bytes
                // This handles pointer-to-pointer assignments (e.g., *ptr = inner_ptr)
                *((void**)ptrVal->data) = (void*)srcStruct;
            } else {
                scope.error("cannot assign value type through pointer dereference in interpret", this);
            }
        } else if(newVal->val_kind() == ValueKind::PointerValue) {
            // Assigning a pointer value through pointer dereference (e.g., *ptr = &raw y)
            if(ptrVal->data && byteSize >= sizeof(void*)) {
                *((void**)ptrVal->data) = ((PointerValue*)newVal)->data;
            }
        } else {
            scope.error("cannot assign value type through pointer dereference in interpret", this);
        }
    } else {
        // Non-pointer inner: implicit reference deref (e.g., local variable access)
        // Delegate to the inner value's set_value to handle assignment through scope
        value->set_value(scope, rawValue, op, location);
    }
}

void DereferenceValue::set_child_value(InterpretScope& scope, const chem::string_view& name, Value* newValue, Operation op) {
    // Evaluate the inner expression to get the pointer
    const auto ptrEval = value->evaluated_value(scope);
    if(!ptrEval) {
        scope.error("cannot set child through dereference: inner value could not be evaluated", this);
        return;
    }
    if(ptrEval->val_kind() == ValueKind::PointerValue) {
        auto ptrVal = (PointerValue*) ptrEval;
        // Write through the pointer directly — access the underlying struct via data
        // and set the child field on it. This avoids going through deref() which
        // creates a copy and would lose modifications.
        if(ptrVal->data) {
            auto structVal = (StructValue*) ptrVal->data;
            auto newVal = newValue->evaluated_value(scope);
            if(newVal) {
                structVal->set_child_value(scope, name, newVal, op);
                return;
            }
        }
    }
    // Fallback: default behavior
    Value::set_child_value(scope, name, newValue, op);
}

DereferenceValue *DereferenceValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<DereferenceValue>()) DereferenceValue(
            value->copy(allocator),
            getType(),
            encoded_location()
    );
}