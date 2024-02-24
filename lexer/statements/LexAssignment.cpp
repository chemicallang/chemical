// Copyright (c) Qinetik 2024.

#include <vector>
#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/tokens/OperatorToken.h"

std::optional<LexError> Lexer::lexAssignmentTokens(std::vector<std::unique_ptr<LexToken>> &tokens){

    // lex an identifier token
    if(!lexIdentifierTokenBool(tokens)) {
        return std::nullopt;
    }

    // whitespace
    lexWhitespaceToken(tokens);

    // =
    if(provider.increment('=')) {
        tokens.emplace_back(std::make_unique<CharOperatorToken>(provider.position() - 1, lineNumber(), '='));
    } else {
        return error("expected equal sign '=' for variable assignment");
    }

    // value
    if(!lexValueToken(tokens)) {
        return error("expected a value for variable assignment");
    }

    // whitespace
    lexWhitespaceToken(tokens);

    return std::nullopt;

}