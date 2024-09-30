// Copyright (c) Qinetik 2024.

#include "Continue.h"
#include "ast/base/InterpretScope.h"

/**
 * @brief Construct a new ContinueStatement object.
 */
ContinueStatement::ContinueStatement(LoopASTNode *node, ASTNode* parent_node, CSTToken* token) : node(node), parent_node(parent_node), token(token) {

}

void ContinueStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void ContinueStatement::interpret(InterpretScope &scope) {
//    if (node == nullptr) {
//        scope.error("[Continue] statement has nullptr to loop node", this);
//        return;
//    }
    node->body.stopInterpretOnce();
}
