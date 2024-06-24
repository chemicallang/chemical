// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ThrowStatement.h"

ThrowStatement::ThrowStatement(std::unique_ptr<Value> value) : value(std::move(value)) {

}

void ThrowStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}