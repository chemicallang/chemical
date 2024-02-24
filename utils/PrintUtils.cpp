// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "PrintUtils.h"

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed) {
    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start() << "," << item->end() << ")";
        if (!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << '\n';
    }
}

void printTokens(const std::vector<SemanticToken> &tokens) {
    for (const auto &item: tokens) {
        std::cout << "[deltaLine]" << item.deltaLine << " - ";
        std::cout << "[deltaStart]" << item.deltaStart << " - ";
        std::cout << "[deltaLength]" << item.length << " - ";
        std::cout << '\n';
    }
}
