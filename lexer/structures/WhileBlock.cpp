// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexWhileBlockTokens() {

    if(!lexKeywordToken("while")) {
        return false;
    }

    lexWhitespaceToken();

    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( after keyword while for while block");
        return true;
    }

    if(!lexConditionalStatement()) {
        error("expected a conditional statement for while block");
        return true;
    }

    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) for while block");
        return true;
    }

    lexWhitespaceToken();

    if(!lexBraceBlock()) {
        error("expected a brace block { statement(s) } when lexing a while block");
    }

    return true;

}