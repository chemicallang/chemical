// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexStructValueTokens() {
    if(lexOperatorToken('{')) {

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto identifier = lexIdentifier();
            if(storeIdentifier(identifier, false)) {
                lexWhitespaceToken();
                if(!lexOperatorToken(':')) {
                    error("expected a ':' for initializing struct member " + identifier);
                    break;
                }
                lexWhitespaceToken();
                if(!lexValueToken()) {
                    error("expected a value after ':' for struct member " + identifier);
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
            error("expected '}' for struct value");
        }

        return true;
    }
    return false;
}