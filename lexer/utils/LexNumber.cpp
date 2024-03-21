// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"

bool Lexer::lexNumberToken() {
    auto number = lexNumber();
    if (!number.empty()) {
        tokens.emplace_back(std::make_unique<NumberToken>(backPosition(number.length()), number));
        return true;
    } else {
        return false;
    }
}