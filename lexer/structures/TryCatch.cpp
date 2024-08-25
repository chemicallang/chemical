// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexTryCatchTokens() {
    if (lexWSKeywordToken("try")) {
        unsigned int start = tokens.size() - 1;
        if(lexAccessChain(false)) {
            lexWhitespaceToken();
            if (lexWSKeywordToken("catch", '(')) {
                if(lexOperatorToken('(')) {
                    lexWhitespaceToken();
                    if(!lexVariableToken()) {
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
                compound_from(start, LexTokenType::CompTryCatch);
            } //optional catch
        } else {
            error("expected '{' after 'try' for a block");
        }
        return true;
    }
    return false;
}