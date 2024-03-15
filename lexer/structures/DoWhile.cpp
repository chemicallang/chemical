// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexDoWhileBlockTokens() {

    if(!lexKeywordToken("do")) {
        return false;
    }

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("dowhileloop")) {
        error("expected a brace block { statement(s) } when lexing a while block");
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    lexWhitespaceToken();

    if(!lexKeywordToken("while")) {
        error("expected 'while' with condition in a do while loop");
        return true;
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

    return true;

}