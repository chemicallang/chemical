//
// Created by wakaz on 10/12/2023.
//

#include <iostream>
#include "Lexer.h"
#include "model/KeywordToken.h"

std::optional<int> Lexer::lexInt(bool intOnly) {
    char current = provider.peek();
    char next = provider.peek(1);
    if((current >= '0' && current <= '9') || (current == '-' && next >= '0' && next <= '9')) {
        bool isNegative = false;
        if(current == '-') {
            provider.readCharacter();
            isNegative = true;
        }
        current = provider.readCharacter(); // this line reads first digit
        int digits = current - '0'; // stores first digit as
        while(!provider.eof()) {
            current = provider.peek();
            if(current >= '0' && current <= '9') {
                current = provider.readCharacter();
                digits *= 10;
                digits += current - '0';
            } else if(!intOnly && current == '.') {
                // TODO handle floating point
                provider.readCharacter();
            }else {
                break;
            }
        }
        if(isNegative) {
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
            auto pos = provider.position();
            auto token = new KeywordToken(pos, pos + 3, "var");
            tokens.push_back(token);
            continue;
        }
        auto lexedInt = lexInt();
        if (lexedInt.has_value()) {
            auto token = new IntToken(pos, provider.position(),  lexedInt.value());
            tokens.push_back(token);
            continue;
        }
        char unhandled = provider.readCharacter();
        std::cout << "Unhandled:" << unhandled << std::endl;
    }
    return tokens;
}