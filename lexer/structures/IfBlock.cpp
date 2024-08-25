// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexIfExprAndBlock() {

    if (!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( when lexing a if block");
        return;
    }

    if (!lexExpressionTokens()) {
        error("expected a conditional expression when lexing a if block");
        return;
    }

    if (!lexOperatorToken(')')) {
        error("expected a ending parenthesis ) when lexing a if block");
        return;
    }

    if (has_errors) {
        return;
    }

    if (!lexBraceBlock("if")) {
        error("expected a brace block when lexing a brace block");
        return;
    }

}

bool Lexer::lexIfBlockTokens() {

    if(!lexWSKeywordToken("if", '(')) {
        return false;
    }

    auto start = tokens.size() - 1;

    lexIfExprAndBlock();
    if (has_errors) {
        return true;
    }

    // lex whitespace
    lexWhitespaceAndNewLines();

    // keep lexing else if blocks until last else appears
    while (lexWSKeywordToken("else", '{')) {
        lexWhitespaceAndNewLines();
        if (provider.peek() == '{') {
            if (!lexBraceBlock("else")) {
                error("expected a brace block after the else while lexing an if statement");
            }
            compound_from(start, LexTokenType::CompIf);
            return true;
        } else {
            if(lexWSKeywordToken("if", '(')) {
                lexIfExprAndBlock();
                lexWhitespaceToken();
            } else {
                error("expected an if statement / brace block after the 'else' but none found");
            }
        }
    }

    compound_from(start, LexTokenType::CompIf);

    return true;

}