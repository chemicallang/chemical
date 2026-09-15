// Copyright (c) Chemical Language Foundation 2026.

#include "AwaitExpression.h"

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

Value* AwaitExpression::evaluated_value(InterpretScope& scope) {
    // Phase 1 interpreter support: `await` is eager/transparent in the
    // bootstrap, so the awaited value is simply the evaluated inner
    // expression. The executor-backed (`Pending`) interpreter path is layered
    // on with the real future protocol.
    return inner->evaluated_value(scope);
}

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/backend/LLVMCoroutine.h"

llvm::Value* AwaitExpression::llvm_value(Codegen& gen, BaseType* expected_type) {
    // Inside a lowered async function on the LLVM backend, `await` drives the
    // child future's poll through its vtable and suspends the coroutine when it
    // is pending (design Section 9.6). This only applies when the operand is a
    // compiler-generated `FutureHandle<T>`: its type differs from the resolved
    // await result type `T`. A *transparent* await (`await v`, `await flag`)
    // yields the operand itself and must not touch the future protocol.
    if(gen.current_coro != nullptr && await_result_type != nullptr && inner->getType() != nullptr) {
        const auto inner_type = const_cast<BaseType*>(inner->getType())->canonical();
        const auto result_type = const_cast<BaseType*>(await_result_type)->canonical();
        if(inner_type != result_type) {
            return gen_llvm_await(gen, this);
        }
    }
    // Eager/transparent bootstrap (interpreter, non-lowered backends): the
    // awaited value is the operand itself.
    return inner->llvm_value(gen, expected_type);
}

llvm::Type* AwaitExpression::llvm_type(Codegen& gen) {
    if(getType() != nullptr) {
        return getType()->llvm_type(gen);
    }
    return inner->llvm_type(gen);
}

#endif
