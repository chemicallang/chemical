// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/RuntimeType.h"

Value* runtime_value_of(InterpretScope& scope, Value* underlying);

/**
 * runtime value, a runtime value is
 */
class RuntimeValue : public Value {
public:
    Value* underlying;
    explicit constexpr RuntimeValue(Value* underlying, RuntimeType* type) : Value(ValueKind::RuntimeValue, type, ZERO_LOC), underlying(underlying) {

    }
    inline RuntimeType* getType() {
        return (RuntimeType*) Value::getType();
    }
    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<RuntimeValue>()) RuntimeValue(underlying->copy(allocator), getType()->copy(allocator));
    }
    Value* evaluated_value(InterpretScope &scope) final {
        return runtime_value_of(scope, underlying);
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