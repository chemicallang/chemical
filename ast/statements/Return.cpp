// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/utils/ASTUtils.h"

ReturnStatement::ReturnStatement(
        std::unique_ptr<Value> value,
        FunctionType *declaration,
        ASTNode* parent_node,
        CSTToken* token
) : value(std::move(value)), func_type(declaration), parent_node(parent_node), token(token) {

}

void ReturnStatement::interpret(InterpretScope &scope) {
    auto decl = func_type->as_function();
    if(!decl) return;
    if (value) {
        decl->set_return(value->return_value(scope));
    } else {
        decl->set_return(nullptr);
    }
}

void ReturnStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    if (value) {
        value->link(linker, this);
        if(func_type->returnType) {
            const auto func = func_type->as_function();
            if(func && func->has_annotation(AnnotationKind::Constructor)) {
                return;
            }
            const auto implicit = func_type->returnType->implicit_constructor_for(value.get());
            if (implicit && implicit != func_type && implicit->parent_node != func_type->parent()) {
                value = call_with_arg(implicit, std::move(value), linker);
                return;
            }
        }
    }
}

void ReturnStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

BaseType* ReturnStatement::known_type() {
    return func_type->returnType.get();
}