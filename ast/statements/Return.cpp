// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"

ReturnStatement::ReturnStatement(
        std::optional<std::unique_ptr<Value>> value,
        BaseFunctionType *declaration
) : value(std::move(value)), func_type(declaration) {

}

void ReturnStatement::interpret(InterpretScope &scope) {
    auto decl = func_type->as_func_decl();
    if(!decl) return;
    if (value.has_value()) {
        decl->set_return(value->get()->return_value(scope));
    } else {
        decl->set_return(nullptr);
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
