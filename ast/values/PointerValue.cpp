// Copyright (c) Chemical Language Foundation 2025.

#include "ast/values/StringValue.h"
#include "PointerValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/StructValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/BaseType.h"
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
    if(pointeeType) {
        auto kind = pointeeType->kind();
        if(kind == BaseTypeKind::Linked || kind == BaseTypeKind::Struct || kind == BaseTypeKind::Union || kind == BaseTypeKind::Generic) {
            if(!pointeeType->get_direct_linked_enum()) {
                auto structVal = (StructValue*) data;
                if(structVal) {
                    return structVal->child(scope, name);
                }
            }
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
            // Dereferencing a pointer to pointer: read the stored pointer value from data.
            // data is raw memory allocated by NewTypedValue for `new *int`, where the user
            // stored a void* via DereferenceValue::set_value (e.g., *x = &raw y).
            // Read back that void* and create a PointerValue pointing to the target data.
            void* storedPtr = *((void**)data);
            auto pointeeType = getType()->as_pointer_type_unsafe()->type;
            if(!storedPtr) {
                return new (scope.allocate<PointerValue>()) PointerValue(
                    nullptr, pointeeType, 0, 0, value_loc
                );
            }
            auto pointeeSize = pointeeType->byte_size(scope.global->target_data);
            return new (scope.allocate<PointerValue>()) PointerValue(
                storedPtr, pointeeType, 0, pointeeSize, value_loc
            );
        }
        case BaseTypeKind::Bool: {
            return new (scope.allocate<BoolValue>()) BoolValue(
                *((char*) data) != 0, typeBuilder.getBoolType(), value_loc
            );
        }
        case BaseTypeKind::Linked: {
            // check if this is an enum (data stores raw int64, not a StructValue*)
            auto enumDecl = getType()->get_direct_linked_enum();
            if(enumDecl) {
                auto rawVal = deref_pointer(data, castedTypeSize);
                return new (scope.allocate<IntNumValue>()) IntNumValue(
                    rawVal,
                    scope.global->typeBuilder.getIntType(),
                    value_loc
                );
            }
            // data was set by AddrOfValue to point to a StructValue
            auto srcStruct = (StructValue*) data;
            if(srcStruct) {
                // Only copy non-destructible (trivially copyable) structs.
                // For destructible structs and variants, return the original
                // since move semantics handle cleanup via move_clear_source.
                auto ext = srcStruct->linked_extendable();
                if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                    auto sd = (StructDefinition*)ext;
                    if(!sd->has_destructor()) {
                        return srcStruct->copy(scope.allocator);
                    }
                }
                return srcStruct; // return original for destructible structs/variants
            }
            return scope.getNullValue();
        }
        case BaseTypeKind::Float: {
            // Read the float value at the pointer location
            float floatVal = 0.0f;
            if(castedTypeSize >= sizeof(float)) {
                floatVal = *((float*)data);
            }
            return new (scope.allocate<FloatValue>()) FloatValue(
                floatVal, 
                (FloatType*)(BaseType*) getType(), 
                value_loc
            );
        }
        case BaseTypeKind::Double: {
            double doubleVal = 0.0;
            if(castedTypeSize >= sizeof(double)) {
                doubleVal = *((double*)data);
            }
            return new (scope.allocate<DoubleValue>()) DoubleValue(
                doubleVal,
                (DoubleType*)(BaseType*) getType(),
                value_loc
            );
        }
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union: {
            auto srcStruct = (StructValue*) data;
            if(srcStruct) {
                auto ext = srcStruct->linked_extendable();
                if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                    auto sd = (StructDefinition*)ext;
                    if(!sd->has_destructor()) {
                        return srcStruct->copy(scope.allocator);
                    }
                }
                return srcStruct;
            }
            return scope.getNullValue();
        }
        case BaseTypeKind::Generic: {
            auto enumDecl = getType()->get_direct_linked_enum();
            if(enumDecl) {
                auto rawVal = deref_pointer(data, castedTypeSize);
                return new (scope.allocate<IntNumValue>()) IntNumValue(
                    rawVal,
                    scope.global->typeBuilder.getIntType(),
                    value_loc
                );
            }
            auto srcStruct = (StructValue*) data;
            if(srcStruct) {
                auto ext = srcStruct->linked_extendable();
                if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                    auto sd = (StructDefinition*)ext;
                    if(!sd->has_destructor()) {
                        return srcStruct->copy(scope.allocator);
                    }
                }
                return srcStruct;
            }
            return scope.getNullValue();
        }
        case BaseTypeKind::Void:
            scope.error("dereferencing to void type not supported in comptime", debugValue ? debugValue : this);
            return nullptr;
        default:
            scope.error("dereferencing to unknown type", debugValue ? debugValue : this);
            return nullptr;
    }
}

void PointerValue::set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op) {
    auto pointeeType = getType();
    if(pointeeType) {
        auto kind = pointeeType->kind();
        if(kind == BaseTypeKind::Linked || kind == BaseTypeKind::Struct || kind == BaseTypeKind::Union || kind == BaseTypeKind::Generic) {
            if(!pointeeType->get_direct_linked_enum()) {
                auto structVal = (StructValue*) data;
                if(structVal) {
                    structVal->set_child_value(scope, name, value, op);
                    return;
                }
            }
        }
    }
    auto dereffed = deref(scope, encoded_location(), this);
    if(dereffed) {
        dereffed->set_child_value(scope, name, value, op);
    }
}