// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/RuntimeType.h"
#include "ast/structures/CapturedComptimeVariable.h"

/**
 * runtime value, a runtime value is
 */
class RuntimeValue : public Value {
public:
    Value* underlying;

    /**
     * the comptime values captured from the enclosing comptime function's
     * scope (parameters/locals referenced inside the underlying expression).
     * the runtime value acts as a template: when evaluated_value is called,
     * a shallow copy is created and the captured values are evaluated into
     * the copy's refs
     */
    CapturedComptimeValues captured_refs;

    explicit constexpr RuntimeValue(Value* underlying, RuntimeType* type) : Value(ValueKind::RuntimeValue, type, ZERO_LOC), underlying(underlying) {

    }
    inline RuntimeType* getType() {
        return (RuntimeType*) Value::getType();
    }
    Value *copy(ASTAllocator& allocator) final {
        auto cp = new (allocator.allocate<RuntimeValue>()) RuntimeValue(underlying->copy(allocator), getType()->copy(allocator));
        cp->captured_refs = captured_refs;
        return cp;
    }
    Value* evaluated_value(InterpretScope &scope) final {
        if (scope.global->interpretation_mode) {
            return underlying->evaluated_value(scope);
        }
        // the returned runtime value references comptime variables captured
        // from the enclosing comptime function's scope (CapturedComptimeVariable
        // bridge nodes). the interpret scope where the return is being
        // interpreted is still alive, so a shallow copy of the runtime value
        // is created (sharing the underlying AST) and the captured values are
        // evaluated into the copy's refs. the copy is fully self-contained and
        // can be translated by the backend after the interpret scope dies. the
        // original keeps acting as a template
        auto* cp = new (scope.allocator.allocate<RuntimeValue>()) RuntimeValue(underlying, getType());
        cp->captured_refs = captured_refs;
        cp->captured_refs.evaluate(scope);
        return cp;
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

#ifdef COMPILER_BUILD
    llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr) override;
#endif
};