// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include <vector>
#include "lexer/model/tokens/LexToken.h"
#include "lexer/Lexer.h"

void Lexer::lexStatementTokens() {
    if (!lexHash || lexHashOperator()) {

        lexVarInitializationTokens();

        lexAssignmentTokens();

    }
}

bool Lexer::lexNewLineChars() {
    auto peak = provider.peek();
    if (peak == '\n') {
        provider.readCharacter();
        return true;
    } else if (peak == '\r') {
        // consuming the \r
        provider.readCharacter();
        // consume also the next \n
        if (provider.peek() == '\n') provider.readCharacter();
        return true;
    } else {
        return false;
    }
}

void Lexer::lexMultipleStatementsTokens() {
    while (!provider.eof() && provider.peek() != EOF && !lexError.has_value()) {
        lexWhitespaceToken();
        lexStatementTokens();
        lexWhitespaceToken();
        if(!lexNewLineChars()) {
            return;
        }
    }
}