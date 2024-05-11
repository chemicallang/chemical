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
        std::string value = "'";
        bool skip_one;
        char read;
        while(true) {
            read = provider.readCharacter();
            value.append(1, read);
            if(read == '\'' && !skip_one) {
                break;
            }
            skip_one = read == '\\';
        };
        tokens.emplace_back(std::make_unique<CharToken>(backPosition(value.size()), std::move(value)));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexBoolToken() {
    if (provider.increment("true")) {
        tokens.emplace_back(std::make_unique<BoolToken>(backPosition(4), "true"));
        return true;
    } else if (provider.increment("false")) {
        tokens.emplace_back(std::make_unique<BoolToken>(backPosition(5), "false"));
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

bool Lexer::lexAccessChainValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken();
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
    return lexAccessChainValueToken() || lexAccessChainOrAddrOf(lexStruct) || lexAnnotationMacro();
}