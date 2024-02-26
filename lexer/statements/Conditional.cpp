// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/StringOperatorToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"

bool Lexer::lexConditionalOperator() {

    if(provider.increment('>')) {
        if(provider.increment('=')) {
            tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), ">="));
        } else {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '>'));
        }
        return true;
    } else if(provider.increment('<')) {
        if(provider.increment('=')) {
            tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(2), "<="));
        } else {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '<'));
        }
        return true;
    } else {
        return false;
    }

}

bool Lexer::lexConditionalStatement() {

    if(!lexAccessChain()) {
        return false;
    }

    lexWhitespaceToken();

    if(!lexConditionalOperator()) {
        error("expected a conditional operator like >,<,>=,<=");
        return false;
    }

    lexWhitespaceToken();

    if(!lexAccessChainOrValue()) {
        error("expected a access chain at right hand side of the conditional operator");
        return false;
    }

    return true;

}