// Copyright (c) Qinetik 2024.

#include "Assignment.h"

AssignStatement::AssignStatement(
        std::unique_ptr<Value> lhs,
        std::unique_ptr<Value> value,
        Operation assOp
) : lhs(std::move(lhs)), value(std::move(value)), assOp(assOp) {}

void AssignStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void AssignStatement::declare_and_link(SymbolResolver &linker) {
    lhs->link(linker, this, true);
    value->link(linker, this, false);
}

void AssignStatement::interpret(InterpretScope &scope) {
    lhs->set_identifier_value(scope, value.get(), assOp);
}