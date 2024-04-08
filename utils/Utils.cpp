// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Utils.h"
#include "common/Diagnostic.h"

#include <iostream>

void printToken(CSTToken *token) {
    std::cout << " - [" << token->type_string() << "]" << "(" << token->start().representation() << ")";
}

void printTokens(const std::vector<std::unique_ptr<CSTToken>> &lexed) {
    for (const auto &item: lexed) {
        printToken(item.get());
        std::cout << std::endl;
    }
}

void printTokens(const std::vector<std::unique_ptr<CSTToken>> &lexed, const std::unordered_map<unsigned int, unsigned int> &linked) {
    int i = 0;
    while(i < lexed.size()) {
        auto found = linked.find(i);
        auto token = found == linked.end() ? lexed[i].get() : lexed[found->second].get();
        printToken(token);
        std::cout << std::endl;
        i++;
    }
}