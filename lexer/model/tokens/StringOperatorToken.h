// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#pragma once

#include "LexToken.h"

/**
 * Its named StringOperator because it holds a string, String has no meaning in terms of syntax
 * The length of this token is always one
 */
class StringOperatorToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitStringOperatorToken(this);
    }

};