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
    lhs->link(linker, this);
    value->link(linker, this);
}

void AssignStatement::interpret(InterpretScope &scope) {
    lhs->set_identifier_value(scope, value.get(), assOp);
}

void AssignStatement::interpret_scope_ends(InterpretScope &scope) {
    // when the var initializer ast node or the holder ast node goes out of scope
    // the newly created value due to function call initializer_value will be destroyed
}

std::string AssignStatement::representation() const {
    std::string rep;
    rep.append(lhs->representation());
    if (assOp != Operation::Assignment) {
        rep.append(" " + to_string(assOp) + "= ");
    } else {
        rep.append(" = ");
    }
    rep.append(value->representation());
    return rep;
}