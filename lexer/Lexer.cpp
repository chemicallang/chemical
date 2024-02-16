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
        str.append(1, x);
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

std::vector<std::unique_ptr<LexToken>> Lexer::lex(const LexConfig &config) {
    std::vector<std::unique_ptr<LexToken>> tokens;
    while (!provider.eof() && provider.peek() != EOF) {
        auto pos = provider.position();
//        std::cout << "Lex Token Session, Character : " << provider.peek() << std::endl;
        if (provider.increment("#")) {
            tokens.emplace_back(std::make_unique<KeywordToken>(pos, 1, lineNumber, "#"));
            continue;
        }
        if (provider.increment("var")) {
            tokens.emplace_back(std::make_unique<KeywordToken>(pos, 3, lineNumber, "var"));
            lexingWhitespace = true;
            continue;
        }
        if (lexingWhitespace) {
            auto whitespace = lexWhitespace();
            if(config.lexWhitespace) {
                tokens.emplace_back(std::make_unique<WhitespaceToken>(pos, whitespace, lineNumber));
            }
            lexingWhitespace = false;
            if(!lexedVariableName) {
                lexingString = true;
            }
            continue;
        }
        if(lexingString) {
            auto str = lexString();
            tokens.emplace_back(std::make_unique<IdentifierToken>(pos, str, lineNumber));
            lexingString = false;
            lexingWhitespace = true;
            lexedVariableName = true;
            lexingEqual = true;
            continue;
        }
        if(lexingEqual) {
            if(provider.increment('=')) {
                tokens.emplace_back(std::make_unique<AssignmentOperatorToken>(pos, 1, lineNumber));
                continue;
            }
            lexingEqual = false;
        }
        if(provider.peek() == ';') {
            provider.readCharacter();
            tokens.emplace_back(std::make_unique<SemiColonToken>(pos, 1, lineNumber));
            continue;
        }
        auto lexedInt = lexInt();
        if (lexedInt.has_value()) {
            tokens.emplace_back(std::make_unique<IntToken>(pos, provider.position(), lineNumber, lexedInt.value()));
            continue;
        }
        char unhandled = provider.readCharacter();
        std::cout << "[Lexer] Unhandled Character : " << unhandled << std::endl;
    }
    return tokens;
}