// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/StringOperatorToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"

bool Lexer::lexComparisonOperators() {

    if (provider.increment("==")) {
        tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), "=="));
    } else if (provider.increment("!=")) {
        tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), "!="));
    } else if (provider.increment('>')) {
        if (provider.increment('=')) {
            tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), ">="));
        } else {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '>'));
        }
    } else if (provider.increment('<')) {
        if (provider.increment('=')) {
            tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), "<="));
        } else {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '<'));
        }
    } else {
        return false;
    }

    return true;

}

bool Lexer::lexConditionalStatement() {

    if (!lexExpressionTokens()) {
        return false;
    }

    lexWhitespaceToken();

    if (!lexComparisonOperators()) {
        return true;
    }

    lexWhitespaceToken();

    if (!lexExpressionTokens()) {
        error("expected a access chain at right hand side of the conditional operator");
        return true;
    }

    return true;

}