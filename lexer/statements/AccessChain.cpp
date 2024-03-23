// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/VariableToken.h"

bool Lexer::storeIdentifier(const std::string& identifier, bool access) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<VariableToken>(backPosition(identifier.length()), identifier, access));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChain(bool access, bool lexStruct) {

    if (!lexIdentifierToken(access)) {
        return false;
    }

    if(lexStruct) {
        lexWhitespaceToken();
        if(provider.peek() == '{') {
            return lexStructValueTokens();
        }
    }

    if (lexOperatorToken('(')) {
        do {
            lexWhitespaceToken();
            if(!lexExpressionTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        if(!lexOperatorToken(')')) {
            error("expected a ')' for a function call, after starting ')'");
        }
    }

    while (lexOperatorToken('[')) {
        lexWhitespaceToken();
        if (!lexExpressionTokens()) {
            error("expected an expression in indexing operators for access chain");
            return true;
        }
        lexWhitespaceToken();
        if (!lexOperatorToken(']')) {
            error("expected a closing bracket ] in access chain");
            return true;
        }
    }

    while (lexOperatorToken('.')) {
        if (!lexAccessChain(access, false)) {
            error("expected a identifier after the dot . in the access chain");
            return true;
        }
    }

    return true;

}