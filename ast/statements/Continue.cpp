// Copyright (c) Qinetik 2024.

#include "Continue.h"

/**
 * @brief Construct a new ContinueStatement object.
 */
ContinueStatement::ContinueStatement(LoopASTNode *node) : node(node) {}

void ContinueStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

#ifdef COMPILER_BUILD

void ContinueStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_continue);
}

#endif

void ContinueStatement::interpret(InterpretScope &scope) {
    if (node == nullptr) {
        scope.error("[Continue] statement has nullptr to loop node");
        return;
    }
    node->body.stopInterpretOnce();
}

std::string ContinueStatement::representation() const {
    std::string ret;
    ret.append("continue;");
    return ret;
}
