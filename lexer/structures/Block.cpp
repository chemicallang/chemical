// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexMultipleStatementsTokens() {
    do {
        do {
            lexWhitespaceToken();
            if (!lexStatementTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(';') && !lexError.has_value());
    } while (lexNewLineChars());
}

bool Lexer::lexBraceBlock() {

    // starting brace
    if(!lexOperatorToken('{')) {
        return false;
    }

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    lexMultipleStatementsTokens();
    prevImportState = prevImportState;

    // ending brace
    if(!lexOperatorToken('}')) {
        error("expected a closing brace }");
        return true;
    }

    return true;

}