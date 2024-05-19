// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Comment : public ASTNode {
public:

    std::string comment;

    bool multiline;

    Comment(std::string comment, bool multiline) : comment(std::move(comment)), multiline(multiline) {}

    void interpret(InterpretScope &scope) override {
        // do nothing, since it's a comment
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {
        // doesn't generate anything
    }
#endif

    std::string representation() const override {
        std::string ret;
        if (multiline) {
            ret.append("/*" + comment + "*/");
        } else {
            ret.append("//" + comment);
        }
        return ret;
    }

};