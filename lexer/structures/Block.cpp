// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexMultipleStatementsTokens() {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        lexWhitespaceAndNewLines();
        if(!lexStatementTokens())  {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

bool Lexer::lexBraceBlock() {

    // whitespace and new lines
    lexWhitespaceAndNewLines();

    // starting brace
    if (!lexOperatorToken('{')) {
        return false;
    }

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    lexMultipleStatementsTokens();
    isLexImportStatement = prevImportState;

    // ending brace
    if (!lexOperatorToken('}')) {
        error("expected a closing brace '}'");
        return true;
    }

    return true;

}