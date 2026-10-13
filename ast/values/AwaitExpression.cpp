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

