// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class StringToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitStringToken(this);
    }

};