// Copyright (c) Qinetik 2024.

#include <vector>
#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"

bool Lexer::lexAssignmentTokens() {

    // lex an identifier token
    if (!lexAccessChain()) {
        return false;
    }

    // increment or decrement
    if (lexOperatorToken("++") || lexOperatorToken("--")) {
        return true;
    }

    // whitespace
    lexWhitespaceToken();


    // lex the operator before the equal sign
    lexOperatorToken('+') ||
    lexOperatorToken('-') ||
    lexOperatorToken('*') ||
    lexOperatorToken('/') ||
    lexOperatorToken('%') ||
    lexOperatorToken('&') ||
    lexOperatorToken('|') ||
    lexOperatorToken('^') ||
    lexOperatorToken("<<") ||
    lexOperatorToken(">>");

    // =
    if (!lexOperatorToken('=')) {
        error("expected equal sign '=' for variable assignment");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!lexValueToken()) {
        error("expected a value for variable assignment");
        return true;
    }

    return true;

}