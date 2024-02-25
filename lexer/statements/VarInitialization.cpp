// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <vector>
#include <memory>
#include "lexer/model/tokens/LexToken.h"
#include "lexer/Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/IdentifierToken.h"
#include "lexer/model/tokens/OperatorToken.h"
#include "utils/FileUtils.h"

bool Lexer::lexVarInitializationTokens() {

    if (!provider.increment("var")) {
        return false;
    }

    // var
    tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(3), "var"));

    // whitespace
    lexWhitespaceToken();

    // identifier
    if (!lexIdentifierTokenBool()) {
        error("expected an identifier for variable initialization");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // :
    if (provider.increment(':')) {

        // operator :
        tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ':'));

        // whitespace
        lexWhitespaceToken();

        // type
        lexTypeTokens();

        // whitespace
        lexWhitespaceToken();

    }

    // equal sign
    if (provider.increment('=')) {
        tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '='));
    } else {

        // lex the optional semicolon when ending declaration
        if (provider.increment(';')) {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ';'));
        }

        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!lexValueToken()) {
        error("expected a value for variable initialization");
        return true;
    }

    return true;

}