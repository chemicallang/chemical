// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/Lexer.h"
#include "cst/values/LambdaCST.h"

void Lexer::lexIdentifierList() {
    do {
        lexWhitespaceAndNewLines();
        if (!lexIdentifierToken()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
}

bool Lexer::lexLambdaValue() {
    if (lexOperatorToken('[')) {

        auto start = tokens.size() - 1;

        lexIdentifierList();

        if (!lexOperatorToken(']')) {
            error("expected ']' after lambda function capture list");
        }

        if (!lexOperatorToken('(')) {
            error("expected '(' for lambda parameter list");
        }

        lexParameterList(true);

        lexNewLineChars();

        if (!lexOperatorToken(')')) {
            error("expected ')' after the lambda parameter list");
        }

        lexWhitespaceToken();

        if (!lexOperatorToken("=>")) {
            error("expected '=>' for a lambda");
        }

        lexWhitespaceToken();

        if (!(lexBraceBlock("lambda") || lexExpressionTokens())) {
            error("expected lambda body");
        }

        compound_from<LambdaCST>(start);

        return true;
    } else {
        return false;
    }
}