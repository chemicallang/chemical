// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharToken.h"

bool Lexer::lexCharToken() {
    if (provider.increment('\'')) {
        auto readChar = provider.readCharacter();
        if (readChar == '\\') {
            auto escaped = provider.escape_sequence();
            readChar = escaped.first;
            if (provider.increment('\'')) {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(4), readChar, 4));
            } else {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(3), readChar, 3));
                error("expected a ' to end a character");
            }
        } else {
            if (provider.increment('\'')) {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(3), readChar, 3));
            } else {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(2), readChar, 2));
                error("expected a ' to end a character");
            }
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexBoolToken() {
    return lexKeywordToken("true") || lexKeywordToken("false");
}

bool Lexer::lexValueToken() {
    return lexCharToken() || lexStringToken() || lexNumberToken() || lexBoolToken();
}

bool Lexer::lexArrayInit() {
    if (lexOperatorToken('[')) {
        do {
            lexWhitespaceToken();
            if (!lexExpressionTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        if (!lexOperatorToken(']')) {
            error("expected a ] when lexing an array");
        }
        lexWhitespaceToken();
        lexTypeTokens();
        lexWhitespaceToken();
        if (lexOperatorToken('(')) {
            do {
                lexWhitespaceToken();
                if (!lexNumberToken()) {
                    break;
                }
                lexWhitespaceToken();
            } while (lexOperatorToken(','));
            lexWhitespaceToken();
            if (!lexOperatorToken(')')) {
                error("expected a ')' when ending array size");
            }
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChainOrValue(bool lexStruct) {
    return lexValueToken() || lexAccessChain(true, lexStruct) || lexAnnotationMacro();
}