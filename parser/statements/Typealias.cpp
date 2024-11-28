// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexTypealiasStatement(unsigned start) {
    if(lexWSKeywordToken(TokenType::TypealiasKw)) {
        if(lexIdentifierToken()) {
            lexWhitespaceToken();
            if(!lexOperatorToken(TokenType::EqualSym)) {
                error("expected '=' after the type tokens");
            }
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a type after '='");
            }
            compound_from(start, LexTokenType::CompTypealias);
        } else {
            error("expected a type for typealias statement");
        }
        return true;
    } else {
        return false;
    }
}