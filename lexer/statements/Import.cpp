// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexImportIdentifierList() {
    if (lexOperatorToken('{')) {
        do {
            lexWhitespaceAndNewLines();
            if (!lexIdentifierToken()) {
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
    if (!lexWSKeywordToken("import")) {
        return false;
    }
    unsigned int start = tokens.size() - 1;
    if (lexStringToken()) {
        if(lexWhitespaceToken() && lexKeywordToken("as")) {
            if(!lexIdentifierToken()) {
                error("expected identifier after 'as' in import statement");
            }
        }
        compound_from(start, LexTokenType::CompImport);
        return true;
    } else {
        if (lexIdentifierToken() || lexImportIdentifierList()) {
            lexWhitespaceToken();
            if (lexKeywordToken("from")) {
                lexWhitespaceToken();
                if (!lexStringToken()) {
                    error("expected path after 'from' in import statement");
                    return false;
                }
            } else {
                error("expected keyword 'from' after the identifier");
                return false;
            }
        } else {
            error("expected a string path in import statement or identifier(s) after the 'import' keyword");
            return false;
        }
        compound_from(start, LexTokenType::CompImport);
    }
    return true;
}