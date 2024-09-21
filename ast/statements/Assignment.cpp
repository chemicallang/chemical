// Copyright (c) Qinetik 2024.

#include "Assignment.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionParam.h"

AssignStatement::AssignStatement(
        Value* lhs,
        Value* value,
        Operation assOp,
        ASTNode* parent_node,
        CSTToken* token
) : lhs(lhs), value(value), assOp(assOp), parent_node(parent_node), token(token) {

}

void AssignStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void AssignStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    lhs->link(linker, lhs, nullptr);
    BaseType* value_type = lhs->create_type(linker.allocator);
    value->link(linker, value, value_type);
    auto id = lhs->as_identifier();
    if(id) {
        auto linked = id->linked_node();
        auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::VarInitStmt) {
            auto init = linked->as_var_init_unsafe();
            init->set_has_assignment();
        } else if(linked_kind == ASTNodeKind::FunctionParam) {
            auto param = linked->as_func_param_unsafe();
            param->set_has_assignment();
        }
    }
    auto& func_type = *linker.current_func_type;
    func_type.mark_moved_value(value, lhs->known_type(), linker, true);
    func_type.mark_un_moved_lhs_value(lhs, lhs->known_type());
}

void AssignStatement::interpret(InterpretScope &scope) {
    lhs->set_identifier_value(scope, value, assOp);
}