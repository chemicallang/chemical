//
// Created by ACER on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/TypeToken.h"

void Lexer::lexTypeTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    auto type = provider.readUntil(' ');
    if (!type.empty()) {
        auto length = type.length();
        tokens.emplace_back(std::make_unique<TypeToken>(provider.position() - length, length, lineNumber(), type));
    }
}