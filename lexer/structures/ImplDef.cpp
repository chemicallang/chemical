// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexImplBlockTokens() {
    do {
        lexWhitespaceToken();
        lexFunctionStructureTokens() || lexVarInitializationTokens();
        lexWhitespaceToken();
        lexOperatorToken(';');
        lexWhitespaceToken();
    } while (lexNewLineChars());
}

bool Lexer::lexImplTokens() {
    if (lexKeywordToken("impl")) {
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected interface name after the interface keyword in implementation");
            return true;
        }
        lexWhitespaceToken();
        if(!lexKeywordToken("for")) {
            error("expected 'for' in impl block after interface name");
            return true;
        }
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected a struct name after the 'for' keyword in implementation");
            return true;
        }
        lexWhitespaceToken();
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an implementation");
            return true;
        }
        lexWhitespaceToken();
        lexNewLineChars();
        lexImplBlockTokens();
        lexWhitespaceAndNewLines();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an implementation");
            return true;
        }
        return true;
    }
    return false;
}