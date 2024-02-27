// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexReturnStatement() {
    if(lexKeywordToken("return")) {
        lexWhitespaceToken();
        lexExpressionTokens();
    } else {
        return false;
    }
}

void Lexer::lexParameterList() {
    do {
        lexWhitespaceToken();
        if(lexIdentifierToken()) {
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

    if(!lexIdentifierToken()) {
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

    lexWhitespaceToken();

    if(lexOperatorToken(':')) {
        lexWhitespaceToken();
        lexTypeTokens();
    }

    return true;

}

bool Lexer::lexFunctionStructureTokens() {

    if(!lexFunctionSignatureTokens()) {
        return false;
    }

    lexWhitespaceToken();

    // inside the block allow return statements
    isLexReturnStatement = true;
    if(!lexBraceBlock()) {
        error("expected the function definition after the signature");
    }
    isLexReturnStatement = false;

    return true;

}