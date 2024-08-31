// Copyright (c) Qinetik 2024.

#include "Assignment.h"

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
}

void AssignStatement::interpret(InterpretScope &scope) {
    lhs->set_identifier_value(scope, value.get(), assOp);
}