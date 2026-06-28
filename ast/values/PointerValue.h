// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/values/StringValue.h"

class ArrayValue;

/**
 * it's sole purpose to simulate pointers in comptime
 */
class PointerValue : public Value {
public:

    // the underlying pointer to data
    void* data;
    // the amount (bytes) pointer can go behind
    size_t behind;
    // the amount (bytes) pointer can go ahead according to the type
    size_t ahead;

    // When a pointer is backed by an array (array-to-pointer decay), this
    // points to the owning ArrayValue and elementIndex tracks the current
    // position. Increment/decrement adjust the index; deref/child access
    // backingArray->values[elementIndex]. This enables `*Point ptr = [a, b]`
    // pattern to work in the interpreter for struct element types where
    // elements are not contiguous in memory.
    ArrayValue* backingArray = nullptr;
    size_t elementIndex = 0;

    /**
     * constructor
     */
    constexpr PointerValue(
        void* data,
        BaseType* type,
        size_t behind,
        size_t ahead,
        SourceLocation location
    ) : Value(ValueKind::PointerValue, type, location), data(data), behind(behind), ahead(ahead) {

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
    ) : Value(ValueKind::PointerValue, type, value->encoded_location()), data((void*) value->value.data()), behind(0) {
        // total bytes ahead = total characters, since 1 char = 1 byte
        ahead = value->value.size();
    }

    PointerValue* copy(ASTAllocator &allocator) override {
        auto pv = new (allocator.allocate<PointerValue>()) PointerValue(
            data, getType(), behind, ahead, encoded_location()
        );
        pv->backingArray = backingArray;
        pv->elementIndex = elementIndex;
        return pv;
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

    Value* child(InterpretScope& scope, const chem::string_view& name) final;

    void set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op) final;

};