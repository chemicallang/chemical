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
    BreakStatement() {}

    void interpret(InterpretScope &scope) override {
        auto current = &scope;
        while(current != nullptr && !current->node->supportsBreak()) {
            current = current->parent;
        }
        if(current == nullptr) {
            scope.error("invalid break statement, couldn't find breakable node up in the tree");
            return;
        }
        current->codeScope->stopInterpretOnce();
        current->node->stopInterpretation();
    }

    std::string representation() const override {
        std::string ret;
        ret.append("break;");
        return ret;
    }

};