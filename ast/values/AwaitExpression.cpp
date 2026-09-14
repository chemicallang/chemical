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

llvm::Value* AwaitExpression::llvm_value(Codegen& gen, BaseType* expected_type) {
    // Phase 0 placeholder: the normalization pass is expected to replace this
    // node before code generation. Forwarding keeps the compiler building and
    // makes the missing lowering obvious (the value produced is the future, not
    // T) if a body ever reaches codegen un-normalized.
    return inner->llvm_value(gen, expected_type);
}

llvm::Type* AwaitExpression::llvm_type(Codegen& gen) {
    if(getType() != nullptr) {
        return getType()->llvm_type(gen);
    }
    return inner->llvm_type(gen);
}

#endif
