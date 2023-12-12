//
// Created by wakaz on 10/12/2023.
//

#include <iostream>
#include "Lexer.h"
#include "model/KeywordToken.h"
#include "model/WhitespaceToken.h"
#include "model/IdentifierToken.h"
#include "model/AssignmentOperatorToken.h"
#include "model/SemiColonToken.h"

int Lexer::lexWhitespace() {
    int whitespaces = 0;
    while (!provider.eof() && provider.peek() == ' ') {
        provider.readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

std::string Lexer::lexString() {
    std::string str;
    while (!provider.eof() && provider.peek() != ' ') {
        char x = provider.readCharacter();
        str.append(&x);
    }
    return str;
}

std::optional<int> Lexer::lexInt(bool intOnly) {
    char current = provider.peek();
    char next = provider.peek(1);
    if ((current >= '0' && current <= '9') || (current == '-' && next >= '0' && next <= '9')) {
        bool isNegative = false;
        if (current == '-') {
            provider.readCharacter();
            isNegative = true;
        }
        current = provider.readCharacter(); // this line reads first digit
        int digits = current - '0'; // stores first digit as
        while (!provider.eof()) {
            current = provider.peek();
            if (current >= '0' && current <= '9') {
                current = provider.readCharacter();
                digits *= 10;
                digits += current - '0';
            } else if (!intOnly && current == '.') {
                // TODO handle floating point
                provider.readCharacter();
            } else {
                break;
            }
        }
        if (isNegative) {
            return -digits;
        } else {
            return digits;
        }
    }
    return std::nullopt;
}

std::vector<LexToken *> Lexer::lex() {
    std::vector<LexToken *> tokens;
    while (!provider.eof() && provider.peek() != EOF) {
        auto pos = provider.position();
//        std::cout << "Lex Token Session, Character : " << provider.peek() << std::endl;
        if (provider.increment("#")) {
            auto token = new KeywordToken(pos, pos + 1, "#");
            tokens.push_back(token);
            continue;
        }
        if (provider.increment("var")) {
            auto token = new KeywordToken(pos, pos + 3, "var");
            tokens.push_back(token);
            lexingWhitespace = true;
            continue;
        }
        if (lexingWhitespace) {
            auto whitespace = lexWhitespace();
            auto token = new WhitespaceToken(pos, pos + whitespace);
            tokens.push_back(token);
            lexingWhitespace = false;
            if(!lexedVariableName) {
                lexingString = true;
            }
            continue;
        }
        if(lexingString) {
            auto str = lexString();
            auto token = new IdentifierToken(pos, str);
            tokens.push_back(token);
            lexingString = false;
            lexingWhitespace = true;
            lexedVariableName = true;
            lexingEqual = true;
            continue;
        }
        if(lexingEqual) {
            if(provider.peek() == '=') {
                provider.readCharacter();
                auto token = new AssignmentOperatorToken(pos, 1);
                tokens.push_back(token);
                continue;
            }
            lexingEqual = false;
        }
        if(provider.peek() == ';') {
            provider.readCharacter();
            auto token = new SemiColonToken(pos, 1);
            tokens.push_back(token);
            continue;
        }
        auto lexedInt = lexInt();
        if (lexedInt.has_value()) {
            auto token = new IntToken(pos, provider.position(), lexedInt.value());
            tokens.push_back(token);
            continue;
        }
        char unhandled = provider.readCharacter();
        std::cout << "Unhandled:" << unhandled << std::endl;
    }
    return tokens;
}