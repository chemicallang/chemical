// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexTryCatchTokens() {
    if (lexWSKeywordToken(TokenType::TryKw)) {
        unsigned int start = tokens_size() - 1;
        if(lexAccessChain(false)) {
            lexWhitespaceToken();
            if (lexWSKeywordToken(TokenType::CatchKw, TokenType::LParen)) {
                if(lexOperatorToken(TokenType::LParen)) {
                    lexWhitespaceToken();
                    if(!lexVariableToken()) {
                        mal_node(start, "expected identifier for 'catch' exception variable");
                        return true;
                    }
                    lexWhitespaceToken();
                    if(!lexOperatorToken(TokenType::ColonSym)) {
                        mal_node(start, "expected ':' after the exception variable identifier in 'catch' block");
                        return true;
                    }
                    lexWhitespaceToken();
                    if(!lexTypeTokens()) {
                        mal_node(start, "expected a type after the ':' in 'catch' block");
                        return true;
                    }
                    lexWhitespaceToken();
                    if(!lexOperatorToken(TokenType::RParen)) {
                        mal_node(start, "expected ')' after the type in 'catch' block");
                        return true;
                    }
                }
                if (!lexBraceBlock("catch")) {
                    mal_node(start, "expected '{' after 'catch' for a block");
                    return true;
                }
                compound_from(start, LexTokenType::CompTryCatch);
            } //optional catch
        } else {
            error("expected a function call after try statement");
        }
        return true;
    }
    return false;
}