// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CommentToken : public LexToken {
public:

    CommentToken(const Position& position, std::string value) : LexToken(position, std::move(value)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitCommentToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Comment;
    }

};