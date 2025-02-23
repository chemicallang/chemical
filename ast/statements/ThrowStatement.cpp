// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ThrowStatement.h"

void ThrowStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}