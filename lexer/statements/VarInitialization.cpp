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

std::optional<LexError> Lexer::lexVarInitializationTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    if (provider.increment("var")) {

        // var
        tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(3), "var"));

        // whitespace
        lexWhitespaceToken(tokens);

        // identifier
        if(!lexIdentifierTokenBool(tokens)) {
            return error("expected an identifier for variable initialization");
        }

        // whitespace
        lexWhitespaceToken(tokens);

        // :
        if(provider.increment(':')) {

            // operator :
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ':'));

            // whitespace
            lexWhitespaceToken(tokens);

            // type
            lexTypeTokens(tokens);

            // whitespace
            lexWhitespaceToken(tokens);

        }

        // equal sign
        if (provider.increment('=')) {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '='));
        } else {

            // lex the optional semicolon when ending declaration
            if (provider.increment(';')) {
                tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ';'));
                return std::nullopt;
            }

            return std::nullopt;
        }

        // whitespace
        lexWhitespaceToken(tokens);

        // value
        if(!lexValueToken(tokens)){
            return error("expected a value for variable initialization");
        }

        // semi colon (optional)
        if (provider.peek() == ';') {
            provider.readCharacter();
            tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), ';'));
        }

    }

    return std::nullopt;

}