// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"

bool Parser::lexStructValueTokens(unsigned back_start) {
    if(lexOperatorToken('{')) {

        unsigned start = tokens_size() - 1 - back_start;

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto identifier = lexIdentifier();
            if(storeIdentifier(identifier)) {
                lexWhitespaceToken();
                if(!lexOperatorToken(':')) {
                    mal_value(start, "expected a ':' for initializing struct member " + identifier);
                    return true;
                }
                lexWhitespaceToken();
                if(!(lexExpressionTokens(true) || lexArrayInit())) {
                    mal_value(start, "expected an expression after ':' for struct member " + identifier);
                    return true;
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
            return true;
        }

        compound_from(start, LexTokenType::CompStructValue);

        return true;
    }
    return false;
}