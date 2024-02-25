// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#include "Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"


std::vector<std::unique_ptr<LexToken>> Lexer::lex(const LexConfig &config) {
    lexMultipleStatementsTokens();
    return std::move(tokens);
}

TokenPosition Lexer::position() {
    return {provider.getLineNumber(), provider.getLineCharNumber(), provider.position()};
}

TokenPosition Lexer::backPosition(unsigned int back) {
    return { provider.getLineNumber(), provider.getLineCharNumber() - back, provider.position() - back };
}