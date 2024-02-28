// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Utils.h"

#include <iostream>

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed) {
    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start() << "," << item->end() << ")";
        if (!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << '\n';
    }
}