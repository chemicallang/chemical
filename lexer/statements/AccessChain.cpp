// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/VariableToken.h"

std::string Lexer::lexIdentifier() {
    if(std::isalpha(provider.peek())) {
        std::string str;
        while (!provider.eof() && (std::isalnum(provider.peek()) || provider.peek() == '_')) {
            str.append(1, provider.readCharacter());
        }
        return str;
    } else {
        return "";
    }
}

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
            lexExpressionTokens();
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        lexOperatorToken(')');
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