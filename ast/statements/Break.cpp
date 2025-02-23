// Copyright (c) Qinetik 2024.

#include "Break.h"
#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

void BreakStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value) {
        value->link(linker, value);
    }
}

void BreakStatement::interpret(InterpretScope &scope) {
    if(node == nullptr) {
        scope.error("[Break] statement has nullptr to loop node", this);
        return;
    }
    node->body.stopInterpretOnce();
    node->stopInterpretation();
}