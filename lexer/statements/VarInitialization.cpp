//
// Created by wakaz on 16/02/2024.
//

#include <vector>
#include <memory>
#include "lexer/model/LexToken.h"
#include "lexer/Lexer.h"
#include "lexer/model/KeywordToken.h"
#include "lexer/model/IdentifierToken.h"
#include "lexer/model/OperatorToken.h"
#include "lexer/model/SemiColonToken.h"

void Lexer::lexVarInitializationTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    if (provider.increment("var")) {

        // var
        tokens.emplace_back(std::make_unique<KeywordToken>(provider.position() - 3, 3, lineNumber(), "var"));

        // whitespace
        lexWhitespaceToken(tokens);

        // identifier
        auto str = lexString();
        tokens.emplace_back(std::make_unique<IdentifierToken>(provider.position() - str.length(), str, lineNumber()));

        // whitespace
        lexWhitespaceToken(tokens);

        // :
        if(provider.increment(':')) {

            // operator :
            tokens.emplace_back(std::make_unique<CharOperatorToken>(provider.position() - 1, 1, lineNumber(), ':'));

            // whitespace
            lexWhitespaceToken(tokens);

            // type
            lexTypeTokens(tokens);

            // whitespace
            lexWhitespaceToken(tokens);

        }

        // equal sign
        if (provider.increment('=')) {
            tokens.emplace_back(std::make_unique<CharOperatorToken>(provider.position() - 1, 1, lineNumber(), '='));
        }

        // whitespace
        lexWhitespaceToken(tokens);

        // integer
        lexIntToken(tokens);

        // semi colon (optional)
        if (provider.peek() == ';') {
            provider.readCharacter();
            tokens.emplace_back(std::make_unique<SemiColonToken>(provider.position() - 1, 1, lineNumber()));
        }

    }
}