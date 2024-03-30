// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexTypealiasStatement() {
    if(lexKeywordToken("typealias")) {
        lexWhitespaceToken();
        if(lexTypeTokens()) {
            lexWhitespaceToken();
            if(!lexOperatorToken('=')) {
                error("expected '=' after the type tokens");
            }
            if(!lexTypeTokens()) {
                error("expected a type after '='");
            }
        } else {
            error("expected a type for typealias statement");
        }
    } else {
        return false;
    }
}