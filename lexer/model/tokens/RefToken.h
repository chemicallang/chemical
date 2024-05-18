// Copyright (c) Qinetik 2024.

#pragma once

#include "LexToken.h"

class RefToken : public LexToken {
public:

    /**
     * the linked token, it can be null
     * that's the difference between identifier and variable tokens, variable
     * tokens can be linked with their definitions (e.g a var init statement)
     * to link the tokens, CSTSymbolResolver is used.
     */
    CSTToken* linked = nullptr;

    using LexToken::LexToken;

};