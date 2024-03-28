// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"

bool Lexer::lexTypeTokens() {
    auto type = lexAnything([&] () -> bool {
        return std::isalpha(provider.peek()) || provider.peek() == '<' || provider.peek() == '>';
    });
    if (!type.empty()) {
        tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
        lexOperatorToken('*');
        return true;
    } else {
        return false;
    }
}