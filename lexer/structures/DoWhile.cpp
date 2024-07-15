// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/DoWhileCST.h"

bool Lexer::lexDoWhileBlockTokens() {

    if(!lexKeywordToken("do")) {
        return false;
    }

    unsigned start = tokens.size() - 1;

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

    compound_from<DoWhileCST>(start, LexTokenType::CompDoWhile);

    return true;

}