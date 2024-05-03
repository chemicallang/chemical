// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharToken.h"
#include "lexer/model/tokens/BoolToken.h"
#include "lexer/model/tokens/NullToken.h"
#include "cst/values/ArrayValueCST.h"

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
    if (provider.increment("true")) {
        tokens.emplace_back(std::make_unique<BoolToken>(backPosition(4), true));
        return true;
    } else if (provider.increment("false")) {
        tokens.emplace_back(std::make_unique<BoolToken>(backPosition(5), false));
        return true;
    }
    return false;
}

bool Lexer::lexNull() {
    if (provider.increment("null")) {
        tokens.emplace_back(std::make_unique<NullToken>(backPosition(4)));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken() || lexBoolToken() ||
           lexNull();
}

bool Lexer::lexArrayInit() {
    if (lexOperatorToken('{')) {
        auto start = tokens.size() - 1;
        do {
            lexWhitespaceAndNewLines();
            if (!(lexExpressionTokens(true) || lexArrayInit())) {
                break;
            }
            lexWhitespaceAndNewLines();
        } while (lexOperatorToken(','));
        if (!lexOperatorToken('}')) {
            error("expected a '}' when lexing an array");
        }
        lexWhitespaceToken();
        if (lexTypeTokens()) {
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
        }
        compound_from<ArrayValueCST>(start);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChainOrValue(bool lexStruct) {
    return lexValueToken() || lexAccessChainOrAddrOf(lexStruct) || lexAnnotationMacro();
}