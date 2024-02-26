// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexIfBlockTokens() {

    if(!lexKeywordToken("if")) {
        return false;
    }

    lexWhitespaceToken();

    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( when lexing a if block");
        return true;
    }

    if(!lexConditionalStatement()) {
        error("expected a conditional statement when lexing a if block");
        return true;
    }

    if(!lexOperatorToken(')')) {
        error("expected a ending parenthesis ) when lexing a if block");
        return true;
    }

    lexWhitespaceToken();

    if(!lexBraceBlock()) {
        error("expected a brace block when lexing a brace block");
    }

    return true;

}