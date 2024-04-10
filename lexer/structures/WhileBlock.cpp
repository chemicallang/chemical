// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/WhileCST.h"

bool Lexer::lexWhileBlockTokens() {

    if(!lexKeywordToken("while")) {
        return false;
    }

    auto start = tokens.size() - 1;

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

    // { statement(s) } with continue & break support
    auto prevLexContinue = isLexContinueStatement;
    auto prevLexBreak = isLexBreakStatement;
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("whileloop")) {
        error("expected a brace block { statement(s) } when lexing a while block");
    }
    isLexContinueStatement = prevLexContinue;
    isLexBreakStatement = prevLexBreak;

    compound_from<WhileCST>(start);

    return true;

}