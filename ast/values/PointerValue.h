// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * it's sole purpose to simulate pointers in comptime
 */
class PointerValue : public Value {
public:

    // the underlying pointer to data
    void* data;
    // the type of data
    BaseType* type;
    // the amount (bytes) pointer can go behind
    size_t behind;
    // the amount (bytes) pointer can go ahead according to the type
    size_t ahead;
    // the location
    SourceLocation location;

    /**
     * constructor
     */
    PointerValue(
        void* data,
        BaseType* type,
        size_t behind,
        size_t ahead,
        SourceLocation location
    ) : data(data), type(type), behind(behind), ahead(ahead), location(location) {

    }

    /**
     * constructor to receive a pointer value when a string value is being casted
     * to a pointer type, however the type passed to this constructor is not a
     * pointer type, it's the pointee type
     */
    PointerValue(
        InterpretScope& scope,
        StringValue* value,
        BaseType* type
    );

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() override {
        return ValueKind::PointerValue;
    }

    void accept(Visitor *visitor) override {
        // cannot be visited
    }

    PointerValue* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<PointerValue>()) PointerValue(
            data, type, behind, ahead, location
        );
    }

    /**
     * cast this pointer value to another type
     */
    PointerValue* cast(InterpretScope& scope, BaseType* type);

    /**
     * increment this pointer value in place
     */
    void increment_in_place(InterpretScope& scope, size_t amount, Value* debugValue);

    /**
     * increment this pointer value in place
     */
    void decrement_in_place(InterpretScope& scope, size_t amount, Value* debugValue);

    /**
     * when user performs an increment
     */
    PointerValue* increment(InterpretScope& scope, size_t amount, SourceLocation location, Value* debugValue);

    /**
     * when user performs a decrement
     */
    PointerValue* decrement(InterpretScope& scope, size_t amount, SourceLocation location, Value* debugValue);

    /**
     * offset is the pointer offset, at zero current pointer
     */
    Value* deref(InterpretScope& scope, SourceLocation location, Value* debugValue);

};