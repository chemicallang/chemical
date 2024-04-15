// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CommentToken : public LexToken {
public:

    std::string value;

    CommentToken(const Position& position, std::string value) : LexToken(position), value(std::move(value)) {
        value.shrink_to_fit();
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    unsigned int length() const override {
        // 2 is added because the token start has // and then comment which is value
        return value.length() + 2;
    }

    LexTokenType type() const override {
        return LexTokenType::Comment;
    }

    void append_representation(std::string &rep) const override {
        rep.append("//");
        rep.append(value);
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Comment:");
        buf.append(value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return value;
    }

};