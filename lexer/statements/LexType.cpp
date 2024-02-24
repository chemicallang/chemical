// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"

void Lexer::lexTypeTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    auto type = provider.readUntil(' ');
    if (!type.empty()) {
        tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
    }
}