// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/BodyCST.h"

void Lexer::lexNestedLevelMultipleStatementsTokens() {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        lexWhitespaceAndNewLines();
        if(!lexNestedLevelStatementTokens())  {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

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

bool Lexer::lexBraceBlock(const std::string& forThing) {

    nested_compound_start();

    // whitespace and new lines
    lexWhitespaceAndNewLines();

    // starting brace
    if (!lexOperatorToken('{')) {
        nested_compound_end();
        return false;
    }

    // multiple statements
    auto prevImportState = isLexImportStatement;
    isLexImportStatement = false;
    nested_compound_start();
    lexNestedLevelMultipleStatementsTokens();
    nested_compound_end();
    isLexImportStatement = prevImportState;

    // ending brace
    if (!lexOperatorToken('}')) {
        error("expected a closing brace '}' for [" + forThing + "]");
        return true;
    }

    compound<BodyCST>();
    nested_compound_end();

    return true;

}