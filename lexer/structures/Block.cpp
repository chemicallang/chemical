// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexMultipleStatementsTokens(bool till_end) {

    // lex whitespace and new lines to reach a statement
    // lex a statement and then optional whitespace, lex semicolon

    while(true) {
        lexWhitespaceAndNewLines();
        if(!lexStatementTokens())  {
            if(till_end) {
                if(provider.eof() || provider.peek() == -1){
                    break;
                } else {
                    // skip to new line
                    auto last_readable_token = tokens.size() - 1;
                    while(!lexNewLineChars() && !(provider.eof() || provider.peek() == -1)) {
                        provider.readCharacter();
                    }
                    error(last_readable_token, "Skipped due to invalid syntax before it");
                    continue;
                }
            } else {
                break;
            }
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    }
}

bool Lexer::lexBraceBlock(const std::string& forThing) {

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
        error("expected a closing brace '}' for [" + forThing + "]");
        return true;
    }

    return true;

}