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

    if(!lexExpressionTokens()) {
        error("expected a conditional statement for while block");
        return true;
    }

    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) for while block");
        return true;
    }

    lexWhitespaceToken();

    // { statement(s) } with continue & break support
    auto prevLexContinue = isLexContinueStatement;
    auto prevLexBreak = isLexBreakStatement;
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock()) {
        error("expected a brace block { statement(s) } when lexing a while block");
    }
    isLexContinueStatement = prevLexContinue;
    isLexBreakStatement = prevLexBreak;

    return true;

}