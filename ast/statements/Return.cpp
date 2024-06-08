// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"

ReturnStatement::ReturnStatement(
        std::optional<std::unique_ptr<Value>> value,
        FunctionDeclaration *declaration
) : value(std::move(value)), declaration(declaration) {

}

void ReturnStatement::interpret(InterpretScope &scope) {
    // TODO lambda returns don't work, since lambda don't correspond to declaration
    if (value.has_value()) {
        declaration->set_return(value->get()->return_value(scope));
    } else {
        declaration->set_return(nullptr);
    }
}

void ReturnStatement::declare_and_link(SymbolResolver &linker) {
    if (value.has_value()) {
        value.value()->link(linker, this);
    }
}

void ReturnStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

ReturnStatement *ReturnStatement::as_return() {
    return this;
}

std::string ReturnStatement::representation() const {
    std::string ret;
    ret.append("return");
    if (value.has_value()) {
        ret.append(1, ' ');
        ret.append(value.value()->representation());
        ret.append(1, ';');
    } else {
        ret.append(1, ';');
    }
    return ret;
}
