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

        lexAssignmentTokens(tokens);

    }
}

bool Lexer::lexNewLineChars() {
    auto peak = provider.peek();
    if (peak == '\n') {
        provider.readCharacter();
        return true;
    } else if(peak == '\r') {
        // consuming the \r
        provider.readCharacter();
        // consume also the next \n
        if(provider.peek() == '\n') provider.readCharacter();
        return true;
    } else {
        return false;
    }
}

void Lexer::lexBodyTokens(std::vector<std::unique_ptr<LexToken>> &tokens) {
    do {
        lexWhitespaceToken(tokens);
        lexStatementTokens(tokens);
        lexWhitespaceToken(tokens);
    } while (lexNewLineChars());
}