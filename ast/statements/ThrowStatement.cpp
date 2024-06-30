// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ThrowStatement.h"

ThrowStatement::ThrowStatement(std::unique_ptr<Value> value, ASTNode* parent_node) : value(std::move(value)), parent_node(parent_node) {

}

void ThrowStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}