// Copyright (c) Qinetik 2025.

#include "ast/values/StringValue.h"
#include "PointerValue.h"
#include "ast/base/InterpretScope.h"

PointerValue::PointerValue(
    InterpretScope& scope,
    StringValue* value,
    BaseType* type
) : type(type), data((void*) value->value.data()), location(value->encoded_location()), behind(0) {
    // total bytes ahead = total characters, since 1 char = 1 byte
    ahead = value->value.size();
}

PointerValue* PointerValue::cast(InterpretScope& scope, BaseType* new_type) {
    return new (scope.allocate<PointerValue>()) PointerValue(
        data, new_type, behind, ahead, location
    );
}

void PointerValue::increment_in_place(InterpretScope& scope, size_t amount, Value* debugValue) {
    const auto castedTypeSize = type->byte_size(scope.isInterpret64Bit());
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes < ahead) {
        data = ((char*) data) + amountBytes;
        behind = behind + amountBytes;
        ahead = ahead - amountBytes;
    } else {
        scope.error("pointer bounds crossed by increment", debugValue ? debugValue : this);
    }
}

void PointerValue::decrement_in_place(InterpretScope& scope, size_t amount, Value* debugValue) {
    const auto castedTypeSize = type->byte_size(scope.isInterpret64Bit());
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= behind) {
        data = ((char*) data) - amountBytes;
        behind = behind - amountBytes;
        ahead = ahead + amountBytes;
    } else {
        scope.error("pointer bounds crossed", debugValue ? debugValue : this);
    }
}

PointerValue* PointerValue::increment(InterpretScope& scope, size_t amount, SourceLocation new_loc, Value* debugValue) {
    const auto castedTypeSize = type->byte_size(scope.isInterpret64Bit());
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes < ahead) {
        return new (scope.allocate<PointerValue>()) PointerValue(
            ((char*) data) + amountBytes, type, behind + amountBytes, ahead - amountBytes, new_loc
        );
    } else {
        scope.error("pointer bounds crossed by increment", debugValue ? debugValue : this);
        return nullptr;
    }
}

PointerValue* PointerValue::decrement(InterpretScope& scope, size_t amount, SourceLocation new_loc, Value* debugValue) {
    const auto castedTypeSize = type->byte_size(scope.isInterpret64Bit());
    const auto amountBytes = castedTypeSize * amount;
    if(amountBytes <= behind) {
        return new (scope.allocate<PointerValue>()) PointerValue(
                ((char*) data) - amountBytes, type, behind - amountBytes, ahead + amountBytes, new_loc
        );
    } else {
        scope.error("pointer bounds crossed", debugValue ? debugValue : this);
        return nullptr;
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

Value* PointerValue::deref(InterpretScope& scope, SourceLocation value_loc, Value* debugValue) {
    const auto castedTypeSize = type->byte_size(scope.isInterpret64Bit());
    if(castedTypeSize > ahead) {
        scope.error("cannot dereference pointer while type size is larger than bytes available", debugValue ? debugValue : this);
        return nullptr;
    }
    switch(type->kind()) {
        case BaseTypeKind::String: {
            return new (scope.allocate<StringValue>()) StringValue(chem::string_view((const char*) data, ahead), value_loc);
        }
        case BaseTypeKind::IntN:{
            const auto intNType = type->as_intn_type_unsafe();
            return intNType->create(scope.allocator, deref_pointer(data, castedTypeSize));
        }
        default:
            scope.error("dereferencing to unknown type", debugValue ? debugValue : this);
            return nullptr;
    }
}