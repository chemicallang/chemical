// Copyright (c) Qinetik 2024.

#include <vector>
#include <memory>
#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"

bool Lexer::lexLanguageOperatorToken() {
    return lexOperatorToken('+') ||
           lexOperatorToken('-') ||
           lexOperatorToken('*') ||
           lexOperatorToken('/') ||
           lexOperatorToken('%') ||
           lexOperatorToken('&') ||
           lexOperatorToken('|') ||
           lexOperatorToken('^') ||
           lexOperatorToken("<<") ||
           lexOperatorToken(">>");
}

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
    lexLanguageOperatorToken();

    // =
    if (!lexOperatorToken('=')) {
        // this is just an expression statement
        // can be a access call
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens() || lexArrayInit())) {
        error("expected a value for variable assignment");
        return true;
    }

    return true;

}