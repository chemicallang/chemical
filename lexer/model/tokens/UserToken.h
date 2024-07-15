// Copyright (c) Qinetik 2024.

#pragma once

#include "LexToken.h"

class LexUserToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitUserToken(this);
    }

};