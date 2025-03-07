// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * CAUTION: wrap value is experimental, if we find a better way to do this, we'll delete it
 * wrap value, wraps a value, so that it can't be evaluated it comptime scope
 * for example, compiler::wrap(x * 3) <-- would produce an expression that if returned
 * from a comptime function would be evaluated at runtime
 * evaluated at runtime
 * wrap value doesn't exist after parsing, it only maybe returned by a comptime
 * function, if we call it, there's no reason to visit this value in visitors
 * because when we call compiler::wrap() <-- this is a comptime function call, that would
 * produce a wrap value, but isn't a wrap value, it's just a function call
 */
class WrapValue : public Value {
public:
    Value* underlying;
    explicit WrapValue(Value* underlying) : Value(ValueKind::WrapValue, ZERO_LOC), underlying(underlying) {

    }
    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<WrapValue>()) WrapValue(underlying->copy(allocator));
    }
    Value* evaluated_value(InterpretScope &scope) final {
        return underlying;
    }
};