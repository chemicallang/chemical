// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class BreakStatement : public ASTNode {
public:

    /**
     * @brief Construct a new Break statement object.
     */
    BreakStatement(LoopASTNode *node) : node(node) {}

    void interpret(InterpretScope &scope) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

private:
    LoopASTNode *node;

};