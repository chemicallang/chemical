// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexTryCatchTokens() {
    if (lexKeywordToken("try")) {
        lexWhitespaceAndNewLines();
        if (lexBraceBlock("try")) {
            lexWhitespaceAndNewLines();
            if (lexKeywordToken("catch")) {
                lexWhitespaceToken();
                if(lexOperatorToken('(')) {
                    lexWhitespaceToken();
                    if(!lexIdentifierToken()) {
                        error("expected identifier for 'catch' exception variable");
                    }
                    lexWhitespaceToken();
                    if(!lexOperatorToken(':')) {
                        error("expected ':' after the exception variable identifier in 'catch' block");
                    }
                    lexWhitespaceToken();
                    if(!lexTypeTokens()) {
                        error("expected a type after the ':' in 'catch' block");
                    }
                    lexWhitespaceToken();
                    if(!lexOperatorToken(')')) {
                        error("expected ')' after the type in 'catch' block");
                    }
                }
                if (!lexBraceBlock("catch")) {
                    error("expected '{' after 'catch' for a block");
                }
            } //optional catch
        } else {
            error("expected '{' after 'try' for a block");
        }
        return true;
    }
    return false;
}