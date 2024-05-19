// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class ContinueStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ContinueStatement object.
     */
    explicit ContinueStatement(LoopASTNode *node);

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope &scope) override;

    std::string representation() const override;

private:
    LoopASTNode *node;

};