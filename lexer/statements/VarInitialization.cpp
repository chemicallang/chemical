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

void Lexer::lexVarInitializationTokens() {
    if (provider.increment("var")) {

        // var
        tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(3), "var"));

        // whitespace
        lexWhitespaceToken();

        // identifier
        if(!lexIdentifierTokenBool()) {
            error("expected an identifier for variable initialization");
            return;
        }

        // whitespace
        lexWhitespaceToken();

        // :
        if(provider.increment(':')) {

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
                return;
            }

            return;
        }

        // whitespace
        lexWhitespaceToken();

        // value
        if(!lexValueToken()){
            error("expected a value for variable initialization");
            return;
        }

        // semi colon (optional)
        if (provider.peek() == ';') {
            provider.readCharacter();
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ';'));
        }

    }

}