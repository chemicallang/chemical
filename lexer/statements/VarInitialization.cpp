// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexVarInitializationTokens(bool allowDeclarations) {

    if (!lexKeywordToken("var")) {
        return false;
    }

    // whitespace
    lexWhitespaceToken();

    // identifier
    if (!lexIdentifierToken(false)) {
        error("expected an identifier for variable initialization");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // :
    if (lexOperatorToken(':')) {

        // whitespace
        lexWhitespaceToken();

        // type
        lexTypeTokens();

        // whitespace
        lexWhitespaceToken();

    }

    // equal sign
    if (!lexOperatorToken('=')) {
        if(!allowDeclarations) {
            error("expected an = sign for the initialization of the variable");
        }
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens(true) || lexArrayInit())) {
        error("expected an expression / array for variable initialization");
        return true;
    }

    return true;

}