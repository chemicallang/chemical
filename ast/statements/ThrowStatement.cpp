// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "ThrowStatement.h"

ThrowStatement::ThrowStatement(
        Value* value,
        ASTNode* parent_node,
        SourceLocation location
) : value(value), parent_node(parent_node), location(location) {

}

void ThrowStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}