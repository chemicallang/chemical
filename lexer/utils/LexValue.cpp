// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharToken.h"

char escape_sequence(char value) {
    char actualChar;
    switch(value) {
        case 'a':
            actualChar = '\a';
            break;
        case 'f':
            actualChar = '\f';
            break;
        case 'r':
            actualChar = '\r';
            break;
        case 'n':
            actualChar = '\n';
            break;
        case '0':
            actualChar = '\0';
            break;
        case 't':
            actualChar = '\t';
            break;
        case 'v':
            actualChar = '\v';
            break;
        case 'b':
            actualChar = '\b';
            break;
        case '\\':
            actualChar = '\\';
            break;
        case '\'':
            actualChar = '\'';
            break;
        case '"':
            actualChar = '\"';
            break;
        case '?':
            actualChar = '\?';
            break;
        default:
            actualChar = value;
            break;
    }
    return actualChar;
}

bool Lexer::lexCharToken() {
    if(provider.increment('\'')) {
        auto readChar = provider.readCharacter();
        if(readChar == '\\') {
            readChar = escape_sequence(provider.readCharacter());
            if(provider.increment('\'')) {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(4), readChar, 4));
            } else {
                tokens.emplace_back(std::make_unique<CharToken>(backPosition(3), readChar, 3));
                error("expected a ' to end a character");
            }
        } else {
            if(provider.increment('\'')) {
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
    if(lexOperatorToken('[')) {
        do {
            lexWhitespaceToken();
            if(!lexExpressionTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(','));
        if(!lexOperatorToken(']')) {
            error("expected a ] when lexing an array");
        }
        lexWhitespaceToken();
        lexTypeTokens();
        lexWhitespaceToken();
        if(lexOperatorToken('(')) {
            do {
                lexWhitespaceToken();
                if(!lexNumberToken()) {
                    break;
                }
                lexWhitespaceToken();
            } while(lexOperatorToken(','));
            lexWhitespaceToken();
            if(!lexOperatorToken(')')) {
                error("expected a ')' when ending array size");
            }
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChainOrValue() {
    return lexValueToken() || lexAccessChain(true);
}