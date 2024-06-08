// Copyright (c) Qinetik 2024.

#include "Typealias.h"
#include "compiler/SymbolResolver.h"

TypealiasStatement::TypealiasStatement(
        std::string from,
        std::unique_ptr<BaseType> to
) : from(std::move(from)), to(std::move(to)) {

}

void TypealiasStatement::interpret(InterpretScope &scope) {

}

void TypealiasStatement::declare_and_link(SymbolResolver &linker) {
    linker.declare(from, this);
    to->link(linker);
}

void TypealiasStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

std::string TypealiasStatement::representation() const {
    return "typealias " + from + " = " + to->representation();
}