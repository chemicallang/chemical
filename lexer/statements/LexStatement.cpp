// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include <vector>
#include "lexer/model/tokens/LexToken.h"
#include "lexer/Lexer.h"

void Lexer::lexStatementTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    if (!lexHash || lexHashOperator(tokens)) {
        lexVarInitializationTokens(tokens);
    }
}
