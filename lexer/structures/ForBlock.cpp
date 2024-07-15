// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/ForLoopCST.h"
#include "cst/statements/ContinueCST.h"
#include "cst/statements/BreakCST.h"

bool Lexer::lexContinueStatement() {
    if(lexKeywordToken("continue")) {
        compound_from<ContinueCST>(tokens.size(), LexTokenType::CompContinue);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexBreakStatement() {
    if(lexKeywordToken("break")) {
        compound_from<BreakCST>(tokens.size(), LexTokenType::CompBreak);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexForBlockTokens() {

    if (!lexKeywordToken("for")) {
        return false;
    }

    unsigned start = tokens.size() - 1;

    // whitespace
    lexWhitespaceToken();

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

    compound_from<ForLoopCST>(start, LexTokenType::CompForLoop);

    return true;

}