// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/statements/ImportCST.h"

bool Lexer::lexImportIdentifierList() {
    if (lexOperatorToken('{')) {
        do {
            lexWhitespaceAndNewLines();
            if (!lexIdentifierToken(false)) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        if (!lexOperatorToken('}')) {
            error("expected a closing bracket '}' after identifier list in import statement");
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexImportStatement() {
    if (!lexKeywordToken("import")) {
        return false;
    }
    lexWhitespaceToken();
    if (lexStringToken()) {
        return true;
    } else {
        if (lexIdentifierToken(false) || lexImportIdentifierList()) {
            lexWhitespaceToken();
            if (lexKeywordToken("from")) {
                lexWhitespaceToken();
                if (!lexStringToken()) {
                    error("expected path after 'from' in import statement");
                }
            } else {
                error("expected keyword 'from' after the identifier");
            }
        } else {
            error("expected a string path in import statement or identifier(s) after the 'import' keyword");
        }
    }
    compound<ImportCST>();
    return true;
}