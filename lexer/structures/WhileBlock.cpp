// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexWhileBlockTokens() {

    if(!lexWSKeywordToken("while", '(')) {
        return false;
    }

    auto start = tokens_size() - 1;

    if(!lexOperatorToken('(')) {
        mal_node(start, "expected a starting parenthesis ( after keyword while for while block");
        return true;
    }

    if(!lexExpressionTokens()) {
        mal_node(start, "expected a conditional statement for while block");
        return true;
    }

    if(!lexOperatorToken(')')) {
        mal_node(start, "expected a closing parenthesis ) for while block");
        return true;
    }

    // { statement(s) } with continue & break support
    auto prevLexContinue = isLexContinueStatement;
    auto prevLexBreak = isLexBreakStatement;
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("whileloop")) {
        mal_node(start, "expected a brace block { statement(s) } when lexing a while block");
        return true;
    }
    isLexContinueStatement = prevLexContinue;
    isLexBreakStatement = prevLexBreak;

    compound_from(start, LexTokenType::CompWhile);

    return true;

}