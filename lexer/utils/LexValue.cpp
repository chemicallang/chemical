// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexValueToken(std::vector<std::unique_ptr<LexToken>> &tokens) {
    return lexIntToken(tokens);
}