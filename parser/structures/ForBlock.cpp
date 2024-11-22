// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexContinueStatement() {
    if(lexWSKeywordToken("continue", ';')) {
        compound_from(tokens_size() - 1, LexTokenType::CompContinue);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexBreakStatement() {
    if(lexWSKeywordToken("break", ';')) {
        auto start = tokens_size() - 1;
        // optionally lex value ahead
        lexAccessChainOrValue();
        compound_from(start, LexTokenType::CompBreak);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexUnreachableStatement() {
    if(lexWSKeywordToken("unreachable", ';')) {
        compound_from(tokens_size() - 1, LexTokenType::CompUnreachable);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexForBlockTokens() {

    if (!lexWSKeywordToken("for", '(')) {
        return false;
    }

    unsigned start = tokens_size() - 1;

    // start parenthesis
    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a for block");
        return true;
    }

    // lex strict initialization
    if(!lexVarInitializationTokens(false)) {
        error("expected a var initialization in for loop");
        return true;
    }

    // lex ;
    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the initialization in for loop");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex conditional expression
    if(!lexExpressionTokens()) {
        error("expected a conditional expression after the var initialization in for loop");
        return true;
    }

    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the condition in for loop");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex assignment token
    if(!lexAssignmentTokens()) {
        error("missing assignment in for loop after the condition");
        return true;
    }

    // end parenthesis
    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) in a for block");
        return true;
    }

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("forloop")) {
        error("expected a brace block in a for block");
        return true;
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    compound_from(start, LexTokenType::CompForLoop);

    return true;

}

bool Parser::lexLoopBlockTokens(bool is_value) {

    if (!lexWSKeywordToken("loop", '{')) {
        return false;
    }

    unsigned start = tokens_size() - 1;

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("forloop")) {
        error("expected a brace block in a for block");
        return true;
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    compound_from(start, is_value ? LexTokenType::CompLoopValue : LexTokenType::CompLoopBlock);

    return true;

}