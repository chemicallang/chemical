//
// Created by wakaz on 13/02/2024.
//

#ifndef CHEMICALVS_PRINTUTILS_H
#define CHEMICALVS_PRINTUTILS_H

#include <vector>
#include <iostream>
#include "lexer/model/LexToken.h"

void printTokens(const std::vector<LexToken *> &lexed) {
    for (const auto &item: lexed) {
        std::cout << " - [" << item->type_string() << "]" << "(" << item->start << "," << item->end << ")";
        if (!item->content().empty()) {
            std::cout << ":" << item->content();
        }
        std::cout << '\n';
    }
}

#endif //CHEMICALVS_PRINTUTILS_H
