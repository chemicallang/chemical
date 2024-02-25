// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"

bool Lexer::lexHashOperator() {
    if (provider.increment("#")) {
        tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(1), "#"));
        return true;
    } else {
        return false;
    }
}