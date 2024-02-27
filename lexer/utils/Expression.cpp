// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexRemainingExpression() {
    lexWhitespaceToken();
    if(!lexLanguageOperatorToken()) {
        return;
    }
    lexWhitespaceToken();
    if(!lexExpressionTokens()) {
        error("expected an expression after the operator token in the expression");
        return;
    }
}

bool Lexer::lexExpressionTokens(){

    if(lexOperatorToken('!')) {
        return lexExpressionTokens();
    }

    if(lexOperatorToken('(')) {

        if(!lexExpressionTokens()) {
            error("expected a nested expression after starting parenthesis ( in the expression");
            return true;
        };

        if(!lexOperatorToken(')')) {
            error("missing ) in the expression");
            return true;
        }

        lexRemainingExpression();

        return true;

    }

    if(!lexAccessChainOrValue()) {
        return false;
    }

    lexRemainingExpression();

    return true;

}