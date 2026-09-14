// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ast/base/Value.h"

/**
 * `await expr` — suspends the enclosing async function until the future
 * produced by `expr` resolves, yielding its result value.
 *
 * After symbol resolution, `await_result_type` holds the inner result type `T`
 * and the node's type is `T`. The operand `inner` has type `S` where either
 * S == FutureHandle<T> or S implements Future<T>.
 *
 * This node is transient: the normalization pass (design Section 7) hoists
 * every AwaitExpression into the initializer of a VarInitStatement before
 * backend code generation.
 */
class AwaitExpression : public Value {

private:

    Value* inner;

public:

    // the result type `T` resolved in symres
    BaseType* await_result_type = nullptr;

    inline AwaitExpression(
        Value* inner,
        SourceLocation location
    ) : Value(ValueKind::AwaitExpr, nullptr, location), inner(inner) {}

    inline Value* getInner() const noexcept {
        return inner;
    }

    inline void setInner(Value* value) {
        inner = value;
    }

    void setAwaitResultType(BaseType* type) {
        await_result_type = type;
        setType(type);
    }

    bool primitive() final {
        return false;
    }

    bool compile_time_computable() override {
        return inner->compile_time_computable();
    }

    Value* copy(ASTAllocator& allocator) override {
        auto copied = new (allocator.allocate<AwaitExpression>())
            AwaitExpression(inner->copy(allocator), encoded_location());
        copied->await_result_type = await_result_type;
        copied->setType(getType());
        return copied;
    }

    ASTNode* linked_node() final {
        return inner->linked_node();
    }

    Value* evaluated_value(InterpretScope& scope) override;

#ifdef COMPILER_BUILD

    // `await` is materialized/normalized before code generation. Until the
    // dedicated lowering lands, these forward to the inner expression so the
    // node is a transparent wrapper (Phase 0). The real lowering replaces the
    // node during normalization.
    llvm::Value* llvm_value(Codegen& gen, BaseType* expected_type = nullptr) final;

    llvm::Type* llvm_type(Codegen& gen) final;

#endif

};
