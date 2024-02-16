//
// Created by wakaz on 16/02/2024.
//

#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/KeywordToken.h"

bool Lexer::lexHashOperator(std::vector<std::unique_ptr<LexToken>> &tokens) {
    if (provider.increment("#")) {
        tokens.emplace_back(std::make_unique<KeywordToken>(provider.position() - 1, 1, lineNumber, "#"));
        return true;
    } else {
        return false;
    }
}