// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Lexer.h"

bool Lexer::lexDoWhileBlockTokens() {

    if(!lexWSKeywordToken("do", '{')) {
        return false;
    }

    unsigned start = tokens_size() - 1;

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("dowhileloop")) {
        mal_node(start, "expected a brace block { statement(s) } when lexing a while block");
        return true;
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    lexWhitespaceToken();

    if(!lexWSKeywordToken("while", '(')) {
        mal_node(start, "expected 'while' with condition in a do while loop");
        return true;
    }

    if(!lexOperatorToken('(')) {
        mal_node(start, "expected a starting parenthesis ( after keyword while for while block");
        return true;
    }

    if(!lexExpressionTokens()) {
        mal_node(start,"expected a conditional statement for while block");
        return true;
    }

    if(!lexOperatorToken(')')) {
        mal_node(start,"expected a closing parenthesis ) for while block");
        return true;
    }

    compound_from(start, LexTokenType::CompDoWhile);

    return true;

}