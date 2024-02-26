// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/IdentifierToken.h"

std::string Lexer::lexIdentifierToken() {
    auto id = lexAlphaNum();
    if(!id.empty()) {
        tokens.emplace_back(std::make_unique<IdentifierToken>(backPosition(id.length()), id));
        return id;
    } else {
        return id;
    }
}

bool Lexer::lexAccessChain() {

    auto id = lexIdentifierToken();
    if(id.empty()) {
        return false;
    }

    while(lexOperatorToken('[')) {
        lexWhitespaceToken();
        if(!lexExpressionTokens()) {
            error("expected an expression in indexing operators for access chain");
            return true;
        }
        lexWhitespaceToken();
        if(!lexOperatorToken(']')) {
            error("expected a closing bracket ] in access chain");
            return true;
        }
    }

    while(lexOperatorToken('.')) {
        if(!lexAccessChain()){
            error("expected a identifier after the dot . in the access chain");
            return true;
        }
    }

    return true;

}