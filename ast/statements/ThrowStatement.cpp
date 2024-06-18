// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ThrowStatement.h"

ThrowStatement::ThrowStatement(std::unique_ptr<Value> value) : value(std::move(value)) {

}

void ThrowStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

std::string ThrowStatement::representation() const {
    std::string rep;
    rep.append("throw ");
    rep.append(value->representation());
    rep.append(1, ';');
    return rep;
}