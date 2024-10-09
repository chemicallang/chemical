// Copyright (c) Qinetik 2024.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"

ReturnStatement::ReturnStatement(
        Value* value,
        FunctionType *declaration,
        ASTNode* parent_node,
        CSTToken* token
) : value(value), func_type(declaration), parent_node(parent_node), token(token) {

}

void ReturnStatement::interpret(InterpretScope &scope) {
    auto decl = func_type->as_function();
    if(!decl) return;
    if (value) {
        decl->set_return(scope, value);
    } else {
        decl->set_return(scope, nullptr);
    }
}

void ReturnStatement::declare_and_link(SymbolResolver &linker) {
    if (value) {
        if(!value->link(linker, value, func_type->returnType ? func_type->returnType : nullptr)) {
            return;
        }
        if(func_type->returnType) {
            const auto func = func_type->as_function();
            if(func && func->has_annotation(AnnotationKind::Constructor)) {
                return;
            }
            const auto implicit = func_type->returnType->implicit_constructor_for(linker.allocator, value);
            if (implicit && implicit != func_type && implicit->parent_node != func_type->parent()) {
                link_with_implicit_constructor(implicit, linker, value);
                return;
            }
            if(!func_type->returnType->satisfies(linker.allocator, value)) {
                linker.unsatisfied_type_err(value, func_type->returnType);
            }
        }
    }
}

void ReturnStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

BaseType* ReturnStatement::known_type() {
    return func_type->returnType;
}