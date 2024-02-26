// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/StringOperatorToken.h"

bool Lexer::lexOperatorToken(char op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), op));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperatorToken(const std::string& op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(op.length()), op));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexKeywordToken(const std::string& keyword) {
    if(provider.increment(keyword)) {
        tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(keyword.length()), keyword));
        return true;
    } else {
        return false;
    }
}


std::string Lexer::lexAnything(char until) {
    return lexAnything([&]() -> bool {
        return provider.peek() != until;
    });
}

std::string Lexer::lexAlphaNum() {
    std::string str;
    while (!provider.eof() && std::isalnum(provider.peek())) {
        str.append(1, provider.readCharacter());
    }
    return str;
}