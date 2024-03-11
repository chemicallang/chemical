// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"

std::string Lexer::lexNumber() {
    auto appearedDot = false;
    auto first_char = true;
    return lexAnything([&]() -> bool {
        auto c = provider.peek();
        if(first_char) {
            first_char = false;
            if(c == '-') {
                return true;
            }
        }
        if (c >= '0' && c <= '9') {
            return true;
        } else if (c == '.' && !appearedDot) {
            appearedDot = true;
            return true;
        } else {
            return false;
        }
    });
}

bool Lexer::lexNumberToken() {
    auto number = lexNumber();
    if (!number.empty()) {
        tokens.emplace_back(std::make_unique<NumberToken>(backPosition(number.length()), number));
        return true;
    } else {
        return false;
    }
}