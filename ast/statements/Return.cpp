// Copyright (c) Chemical Language Foundation 2025.

#include "Return.h"
#include "ast/structures/FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "ast/base/LoopASTNode.h"

bool isLoopNode(ASTNodeKind k) {
    switch(k) {
        case ASTNodeKind::WhileLoopStmt:
        case ASTNodeKind::DoWhileLoopStmt:
        case ASTNodeKind::ForLoopStmt:
        case ASTNodeKind::LoopBlock:
            return true;
        default:
            return false;
    }
}

LoopASTNode* asLoopNode(ASTNode* node) {
    return isLoopNode(node->kind()) ? node->as_loop_node_unsafe() : nullptr;
}

void stop_interpretation_above(ASTNode* node) {
    const auto p = asLoopNode(node);
    if(p) {
        p->stopInterpretation();
    }
    const auto parent = node->parent();
    if(parent) {
        stop_interpretation_above(parent);
    }
}

void ReturnStatement::interpret(InterpretScope &scope) {
    func_type->set_return(scope, value);
    stop_interpretation_above(parent_node);
}

void ReturnStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if (value) {
        if(!value->link(linker, value, func_type->returnType ? func_type->returnType : nullptr)) {
            return;
        }
        if(func_type->data.signature_resolved && func_type->returnType) {
            const auto func = func_type->as_function();
            if(func && func->is_constructor_fn()) {
                return;
            }
            const auto implicit = func_type->returnType->implicit_constructor_for(linker.allocator, value);
            if (implicit && implicit != func_type && implicit->parent_node != func_type->parent()) {
                link_with_implicit_constructor(implicit, linker, value);
                return;
            }
            if(!func_type->returnType->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, func_type->returnType);
            }
        }
    }
}

BaseType* ReturnStatement::known_type() {
    return func_type->returnType;
}