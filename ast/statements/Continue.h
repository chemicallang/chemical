// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class ContinueStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ContinueStatement object.
     */
    ContinueStatement(LoopASTNode *node) : node(node) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {
        gen.CreateBr(gen.current_loop_continue);
    }
#endif

    void interpret(InterpretScope &scope) override {
        if(node == nullptr) {
            scope.error("[Continue] statement has nullptr to loop node");
            return;
        }
        node->body.stopInterpretOnce();
    }

    std::string representation() const override {
        std::string ret;
        ret.append("continue;");
        return ret;
    }

private:
    LoopASTNode *node;

};