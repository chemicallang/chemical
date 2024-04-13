// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"
#include "cst/values/StructValueCST.h"

bool Lexer::lexStructValueTokens() {
    if(lexOperatorToken('{')) {

        unsigned start = tokens.size() - 2;

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
                if(!lexExpressionTokens(true)) {
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
            error("expected '}' for struct value");
        }

        compound_from<StructValueCST>(start);

        return true;
    }
    return false;
}