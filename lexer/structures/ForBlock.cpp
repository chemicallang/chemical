// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexBraceBlock() {

    // starting brace
    if(!lexOperatorToken('{')) {
        return false;
    }

    // multiple statements
    lexMultipleStatementsTokens();

    // ending brace
    if(!lexOperatorToken('}')) {
        error("expected a closing brace }");
        return true;
    }

}

bool Lexer::lexForBlockTokens() {

    if (!lexKeywordToken("for")) {
        return false;
    }

    // whitespace
    lexWhitespaceToken();

    // start parenthesis
    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a for block");
    }

    // optional #
    lexHashOperator();

    // lex strict initialization
    lexVarInitializationTokens(false);

    // lex ;
    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the initialization in for block");
        return true;
    }

    // lex conditional statement
    lexConditionalStatement();

    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the initialization in for block");
        return true;
    }

    // lex assignment token
    if(!lexAssignmentTokens()) {
        error("missing assignment in for block");
        return true;
    }

    // end parenthesis
    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) in a for block");
    }

    // whitespace
    lexWhitespaceToken();

    // { statement }
    if(!lexBraceBlock()) {
        error("expected a brace block in a for block");
    }

    return true;

}