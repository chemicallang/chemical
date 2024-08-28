// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexIfExprAndBlock(bool is_value, bool lex_value_node) {

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

    if (!lexBraceBlockOrSingleStmt("if", is_value, lex_value_node)) {
        error("expected a brace block when lexing a brace block");
        return;
    }

}

bool Lexer::lexIfBlockTokens(bool is_value, bool lex_value_node) {

    if(!lexWSKeywordToken("if", '(')) {
        return false;
    }

    auto start = tokens_size() - 1;

    lexIfExprAndBlock(is_value, lex_value_node);
    if (has_errors) {
        return true;
    }

    // lex whitespace
    lexWhitespaceAndNewLines();

    // keep lexing else if blocks until last else appears
    while (lexWSKeywordToken("else", '{')) {
        lexWhitespaceAndNewLines();
        if(lexWSKeywordToken("if", '(')) {
            lexIfExprAndBlock(is_value, lex_value_node);
            lexWhitespaceToken();
        } else {
            if (!lexBraceBlockOrSingleStmt("else", is_value, lex_value_node)) {
                error("expected a brace block after the else while lexing an if statement");
                return true;
            }
            compound_from(start, is_value ? LexTokenType::CompIfValue :  LexTokenType::CompIf);
            return true;
        }
    }

    compound_from(start, is_value ? LexTokenType::CompIfValue :  LexTokenType::CompIf);

    return true;

}