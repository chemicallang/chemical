// Copyright (c) Qinetik 2024.

#include <vector>
#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/tokens/OperatorToken.h"

void Lexer::lexAssignmentTokens(){

    // lex an identifier token
    if(!lexIdentifierTokenBool()) {
        return;
    }

    // whitespace
    lexWhitespaceToken();

    // =
    if(provider.increment('=')) {
        tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), '='));
    } else {
        error("expected equal sign '=' for variable assignment");
        return;
    }

    // value
    if(!lexValueToken()) {
        error("expected a value for variable assignment");
        return;
    }

    // whitespace
    lexWhitespaceToken();

}