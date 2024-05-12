// Copyright (c) Qinetik 2024.

#include "Break.h"
#include "ast/base/LoopASTNode.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"

void BreakStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_exit);
}

#endif

void BreakStatement::interpret(InterpretScope &scope) {
    if(node == nullptr) {
    scope.error("[Break] statement has nullptr to loop node");
    return;
    }
    node->body.stopInterpretOnce();
    node->stopInterpretation();
}