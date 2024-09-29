// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexStructValueTokens(unsigned back_start) {
    if(lexOperatorToken('{')) {

        unsigned start = tokens_size() - 1 - back_start;

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto identifier = lexIdentifier();
            if(storeIdentifier(identifier)) {
                lexWhitespaceToken();
                if(!lexOperatorToken(':')) {
                    error("expected a ':' for initializing struct member " + identifier);
                    break;
                }
                lexWhitespaceToken();
                if(!(lexExpressionTokens(true) || lexArrayInit())) {
                    error("expected an expression after ':' for struct member " + identifier);
                    break;
                }
                lexWhitespaceToken();
                lexOperatorToken(',');
            } else {
                break;
            }
        } while(true);

        lexWhitespaceAndNewLines();

        if(!lexOperatorToken('}')) {
            mal_value(start, "expected '}' for struct value");
        }

        compound_from(start, LexTokenType::CompStructValue);

        return true;
    }
    return false;
}