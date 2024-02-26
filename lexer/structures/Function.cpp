// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexParameterList() {
    do {
        lexWhitespaceToken();
        if(lexIdentifierTokenBool()) {
            lexWhitespaceToken();
            if(!lexOperatorToken(':')) {
                error("missing a type for the function parameter, expected a colon after the name");
                return;
            }
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("missing a type token for the function parameter, expected type after the colon");
                return;
            }
        }
        lexWhitespaceToken();
    } while(lexOperatorToken(','));
}

bool Lexer::lexFunctionSignatureTokens() {

    if(!lexKeywordToken("func")) {
        return false;
    }

    lexWhitespaceToken();

    if(!lexIdentifierTokenBool()) {
        error("function name is missing, when defining a function");
        return true;
    }

    lexWhitespaceToken();

    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a function signature");
        return true;
    }

    lexParameterList();

    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) when ending a function signature");
        return true;
    }

    return true;

}

bool Lexer::lexFunctionStructureTokens() {

    if(!lexFunctionSignatureTokens()) {
        return false;
    }

    lexWhitespaceToken();

    if(!lexBraceBlock()) {
        error("expected the function definition after the signature");
    }

    return true;

}