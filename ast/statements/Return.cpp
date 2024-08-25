// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/utils/ASTUtils.h"

ReturnStatement::ReturnStatement(
        std::optional<std::unique_ptr<Value>> value,
        FunctionType *declaration,
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

void ReturnStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    if (value.has_value()) {
        value.value()->link(linker, this);
        if(func_type->returnType) {
            const auto implicit = func_type->returnType->implicit_constructor_for(value.value().get());
            if (implicit && implicit != func_type && implicit->parent_node != func_type->parent()) {
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

BaseType* ReturnStatement::known_type() {
    return func_type->returnType.get();
}