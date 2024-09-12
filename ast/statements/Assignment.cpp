// Copyright (c) Qinetik 2024.

#include "Assignment.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"

AssignStatement::AssignStatement(
        std::unique_ptr<Value> lhs,
        std::unique_ptr<Value> value,
        Operation assOp,
        ASTNode* parent_node,
        CSTToken* token
) : lhs(std::move(lhs)), value(std::move(value)), assOp(assOp), parent_node(parent_node), token(token) {

}

void AssignStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void AssignStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    lhs->link(linker, this, true);
    value->link(linker, this, false);
    auto& func_type = *linker.current_func_type;
    func_type.mark_moved_value(value.get(), lhs->known_type(), linker, true);
}

void AssignStatement::interpret(InterpretScope &scope) {
    lhs->set_identifier_value(scope, value.get(), assOp);
}