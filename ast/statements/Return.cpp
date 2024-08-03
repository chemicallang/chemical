// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/utils/ASTUtils.h"

ReturnStatement::ReturnStatement(
        std::optional<std::unique_ptr<Value>> value,
        BaseFunctionType *declaration,
        ASTNode* parent_node
) : value(std::move(value)), func_type(declaration), parent_node(parent_node) {

}

void ReturnStatement::interpret(InterpretScope &scope) {
    auto decl = func_type->as_function();
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
        if(func_type->returnType) {
            const auto implicit = implicit_constructor_for(func_type->returnType.get(), value.value().get());
            if (implicit) {
                value.emplace(call_with_arg(implicit, std::move(value.value()), linker));
                return;
            }
        }
    }
}

void ReturnStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

ReturnStatement *ReturnStatement::as_return() {
    return this;
}
