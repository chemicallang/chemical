// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * CAUTION: wrap value is experimental, if we find a better way to do this, we'll delete it
 * wrap value, wraps a value, so that it can't be evaluated it comptime scope
 * for example, intrinsics::wrap(x * 3) <-- would produce an expression that if returned
 * from a comptime function would be evaluated at runtime
 * evaluated at runtime
 * wrap value doesn't exist after parsing, it only maybe returned by a comptime
 * function, if we call it, there's no reason to visit this value in visitors
 * because when we call intrinsics::wrap() <-- this is a comptime function call, that would
 * produce a wrap value, but isn't a wrap value, it's just a function call
 */
class WrapValue : public Value {
public:
    Value* underlying;
    explicit constexpr WrapValue(Value* underlying) : Value(ValueKind::WrapValue, underlying->getType(), ZERO_LOC), underlying(underlying) {

    }
    explicit constexpr WrapValue(Value* underlying, BaseType* type) : Value(ValueKind::WrapValue, type, ZERO_LOC), underlying(underlying) {

    }
    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<WrapValue>()) WrapValue(underlying->copy(allocator), getType()->copy(allocator));
    }
    Value* evaluated_value(InterpretScope &scope) final {
        return underlying;
    }

    Value* child(InterpretScope& scope, const chem::string_view& name) final {
        const auto eval = underlying->evaluated_value(scope);
        return eval ? eval->child(scope, name) : nullptr;
    }

    Value* call_member(InterpretScope& scope, const chem::string_view& name, std::vector<Value*>& values) final {
        const auto eval = underlying->evaluated_value(scope);
        return eval ? eval->call_member(scope, name, values) : nullptr;
    }

    Value* index(InterpretScope& scope, int i) final {
        const auto eval = underlying->evaluated_value(scope);
        return eval ? eval->index(scope, i) : nullptr;
    }

    Value* find_in(InterpretScope& scope, Value* parent) final {
        const auto eval = underlying->evaluated_value(scope);
        return eval ? eval->find_in(scope, parent) : nullptr;
    }
};