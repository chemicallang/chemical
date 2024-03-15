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

    void interpret(InterpretScope &scope) override {
        if(node == nullptr) {
            std::cerr << "[Break] statement has nullptr to loop node";
            return;
        }
        node->body.stopInterpretOnce();
        node->stopInterpretation();
    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void code_gen(Codegen &gen) override {
        gen.CreateBr(gen.current_loop_exit);
    }

    std::string representation() const override {
        std::string ret;
        ret.append("break;");
        return ret;
    }

private:
    LoopASTNode *node;

};