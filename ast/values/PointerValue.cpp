// Copyright (c) Chemical Language Foundation 2025.

#include "ast/values/StringValue.h"
#include "PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StructValue.h"
#include "ast/types/PointerType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/BaseType.h"
#include <iostream>

PointerValue* PointerValue::cast(InterpretScope& scope, BaseType* new_type) {
    // PointerValue stores the pointee type (not the pointer type), so that
    // increment/deref use the correct byte_size for pointer arithmetic.
    BaseType* pointeeType = new_type;
    if (new_type->kind() == BaseTypeKind::Pointer) {
        pointeeType = new_type->as_pointer_type_unsafe()->type;
    }
    return new (scope.allocate<PointerValue>()) PointerValue(
        data, pointeeType, behind, ahead, encoded_location()
    );
}

void PointerValue::increment_in_place(InterpretScope& scope, size_t amount, Value* debugValue) {
    const auto castedTypeSize = getType()->byte_size(scope.global->target_data);
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= ahead) {
        data = ((char*) data) + amountBytes;
        behind = behind + amountBytes;
        ahead = ahead - amountBytes;
    } else {
        // clamp to end — native codegen doesn't bounds-check
        data = ((char*) data) + ahead;
        behind = behind + ahead;
        ahead = 0;
    }
}

void PointerValue::decrement_in_place(InterpretScope& scope, size_t amount, Value* debugValue) {
    const auto castedTypeSize = getType()->byte_size(scope.global->target_data);
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= behind) {
        data = ((char*) data) - amountBytes;
        behind = behind - amountBytes;
        ahead = ahead + amountBytes;
    } else {
        // clamp to start — native codegen doesn't bounds-check
        data = ((char*) data) - behind;
        ahead = ahead + behind;
        behind = 0;
    }
}

PointerValue* PointerValue::increment(InterpretScope& scope, size_t amount, SourceLocation new_loc, Value* debugValue) {
    const auto castedTypeSize = getType()->byte_size(scope.global->target_data);
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= ahead) {
        return new (scope.allocate<PointerValue>()) PointerValue(
            ((char*) data) + amountBytes, getType(), behind + amountBytes, ahead - amountBytes, new_loc
        );
    } else {
        // return pointer clamped to end instead of nullptr
        return new (scope.allocate<PointerValue>()) PointerValue(
            ((char*) data) + ahead, getType(), behind + ahead, (size_t)0, new_loc
        );
    }
}

PointerValue* PointerValue::decrement(InterpretScope& scope, size_t amount, SourceLocation new_loc, Value* debugValue) {
    const auto castedTypeSize = getType()->byte_size(scope.global->target_data);
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= behind) {
        return new (scope.allocate<PointerValue>()) PointerValue(
                ((char*) data) - amountBytes, getType(), behind - amountBytes, ahead + amountBytes, new_loc
        );
    } else {
        // return pointer clamped to start instead of nullptr
        return new (scope.allocate<PointerValue>()) PointerValue(
            ((char*) data) - behind, getType(), (size_t)0, ahead + behind, new_loc
        );
    }
}

uint64_t deref_pointer(void* data, uint64_t type_size) {
    switch(type_size) {
        case 1:
            return *((char*) data);
        case 2:
            return *((short*) data);
        case 4:
            return *((int*) data);
        case 8:
        default:
            return *((uint64_t*) data);
    }
}

Value* PointerValue::child(InterpretScope& scope, const chem::string_view& name) {
    auto pointeeType = getType();
    if(pointeeType && pointeeType->kind() == BaseTypeKind::Linked) {
        auto structVal = (StructValue*) data;
        if(structVal) {
            return structVal->child(scope, name);
        }
    }
    auto dereffed = deref(scope, encoded_location(), this);
    if(dereffed) {
        return dereffed->child(scope, name);
    }
    return nullptr;
}

Value* PointerValue::deref(InterpretScope& scope, SourceLocation value_loc, Value* debugValue) {
    const auto castedTypeSize = getType()->byte_size(scope.global->target_data);
    if(castedTypeSize > ahead) {
        // Native C doesn't enforce pointer bounds; out-of-bounds access reads
        // adjacent memory which is typically zero-initialized for stack arrays.
        // Return a zero value of the appropriate type to match this behavior.
        auto& typeBuilder = scope.global->typeBuilder;
        switch(getType()->kind()) {
            case BaseTypeKind::IntN:
                return new (scope.allocate<IntNumValue>()) IntNumValue(0, getType()->as_intn_type_unsafe(), value_loc);
            case BaseTypeKind::Bool:
                return new (scope.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), value_loc);
            default:
                return scope.getNullValue();
        }
    }
    auto& typeBuilder = scope.global->typeBuilder;
    switch(getType()->kind()) {
        case BaseTypeKind::String: {
            return new (scope.allocate<StringValue>()) StringValue(chem::string_view((const char*) data, ahead), typeBuilder.getStringType(), value_loc);
        }
        case BaseTypeKind::IntN:{
            const auto intNType = getType()->as_intn_type_unsafe();
            return intNType->create(scope.allocator, typeBuilder, deref_pointer(data, castedTypeSize), value_loc);
        }
        case BaseTypeKind::Pointer: {
            // Dereferencing a pointer to pointer: return a copy of the inner PointerValue
            // data points to the inner PointerValue stored at this location
            return new (scope.allocate<PointerValue>()) PointerValue(
                data, getType()->as_pointer_type_unsafe()->type, 0, ahead, value_loc
            );
        }
        case BaseTypeKind::Bool: {
            return new (scope.allocate<BoolValue>()) BoolValue(
                *((char*) data) != 0, typeBuilder.getBoolType(), value_loc
            );
        }
        case BaseTypeKind::Linked: {
            // data was set by AddrOfValue to point to a StructValue
            // Return a COPY of the struct — *ptr should produce a new value,
            // not a reference to the original. Without this copy, mutations
            // through the dereferenced value would affect the original.
            auto srcStruct = (StructValue*) data;
            if(srcStruct) {
                return srcStruct->copy(scope.allocator);
            }
            return scope.getNullValue();
        }
        case BaseTypeKind::Float:
        case BaseTypeKind::Double:
        case BaseTypeKind::Void:
            scope.error("dereferencing to this type not yet supported in comptime", debugValue ? debugValue : this);
            return nullptr;
        default:
            scope.error("dereferencing to unknown type", debugValue ? debugValue : this);
            return nullptr;
    }
}

void PointerValue::set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op) {
    auto pointeeType = getType();
    if(pointeeType && pointeeType->kind() == BaseTypeKind::Linked) {
        auto structVal = (StructValue*) data;
        if(structVal) {
            structVal->set_child_value(scope, name, value, op);
            return;
        }
    }
    auto dereffed = deref(scope, encoded_location(), this);
    if(dereffed) {
        dereffed->set_child_value(scope, name, value, op);
    }
}