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
        compound_from<ContinueCST>(tokens.size());
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexBreakStatement() {
    if(lexKeywordToken("break")) {
        compound_from<BreakCST>(tokens.size());
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
    }

    // lex strict initialization
    lexVarInitializationTokens(false);

    // lex ;
    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the initialization in for block");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex conditional expression
    lexExpressionTokens();

    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the condition in for block");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex assignment token
    if(!lexAssignmentTokens()) {
        error("missing assignment in for block");
        return true;
    }

    // end parenthesis
    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) in a for block");
    }

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("forloop")) {
        error("expected a brace block in a for block");
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    compound_from<ForLoopCST>(start);

    return true;

}