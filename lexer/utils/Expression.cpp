// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexRemainingExpression(unsigned start) {

    lexWhitespaceToken();
    if (lexWSKeywordToken("as")) {
        if (!lexTypeTokens()) {
            error("expected a type for casting after 'as' in expression");
            return true;
        }
        compound_from(start, LexTokenType::CompCastValue);
        return true;
    } else if(lexWSKeywordToken("is")) {
        if (!lexTypeTokens()) {
            error("expected a type for IsValue after 'is'");
            return true;
        }
        compound_from(start, LexTokenType::CompIsValue);
        return true;
    }
    if (!lexLanguageOperatorToken()) {
        return false;
    }
    lexWhitespaceToken();
    if (!lexExpressionTokens(false, false)) {
        error("expected an expression after the operator token in the expression");
        return true;
    }

    compound_from(start, LexTokenType::CompExpression);
    return true;

}

// lexes lambda after comma which occurs after a parameter param : type,  <-----
// this can be called after lparen to lex lambda, if it has no parameter
bool condLexLambdaAfterComma(Lexer *lexer, unsigned int start) {
    lexer->lexNewLineChars();
    if (!lexer->lexOperatorToken(')')) {
        return false;
    }
    lexer->lexLambdaAfterParamsList(start);
    return true;
}

void lexLambdaAfterComma(Lexer *lexer, unsigned int start) {
    if (!condLexLambdaAfterComma(lexer, start)) {
        lexer->error("expected ')' after the lambda parameter list in parenthesized expression");
    }
}

bool Lexer::lexLambdaOrExprAfterLParen() {
    unsigned int start = tokens.size() - 1;

    lexWhitespaceToken();

    // a lambda with no params
    if (condLexLambdaAfterComma(this, start)) {
        return true;
    }

    if (!lexVariableToken()) {
        return false;
    }

    bool has_whitespace = lexWhitespaceToken();

    if (provider.peek() == ')') {
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        lexOperatorToken(')');
        lexLambdaAfterParamsList(start);
        return true;
    } else if (lexOperatorToken(':')) {
        lexWhitespaceToken();
        if (lexTypeTokens()) {
            lexWhitespaceToken();
        } else {
            error("expected a type after ':' when lexing a lambda in parenthesized expression");
        }
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        if (lexOperatorToken(',')) {
            lexParameterList(true, false);
        }
        lexLambdaAfterComma(this, start);
        return true;
    } else if (provider.peek() == ',') {
        compound_from(start + 1, LexTokenType::CompFunctionParam);
        lexOperatorToken(',');
        lexParameterList(true, false);
        lexLambdaAfterComma(this, start);
        return true;
    }

    if(has_whitespace) {
        lexRemainingExpression(start + 1);
        if(lexOperatorToken(')')) {
            if(!lexRemainingExpression(start)) {
                compound_from(start, LexTokenType::CompExpression);
            }
            return true;
        } else if(lexRemainingExpression(start + 1) && lexOperatorToken(')')) {
            compound_from(start, LexTokenType::CompExpression);
        } else {
            error("expected ')' after the nested parenthesized expression");
        }
        return true;
    } else {
        lexAccessChainAfterId();
        if(!tokens[start + 1]->is_struct_value()) {
            compound_from(start + 1, LexTokenType::CompAccessChain);
        }
        lexRemainingExpression(start);
        if(!lexOperatorToken(')')) {
            error("expected a ')' after the access chain");
        }
        return true;
    }

}

void Lexer::lexParenExpressionAfterLParen() {

    if (!lexExpressionTokens(false, false)) {
        error("expected a nested expression after starting parenthesis ( in the expression");
        return;
    };

    if (!lexOperatorToken(')')) {
        error("missing ) in the expression");
        return;
    }

}

bool Lexer::lexParenExpression() {
    if (lexOperatorToken('(')) {
        lexParenExpressionAfterLParen();
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexExpressionTokens(bool lexStruct, bool lambda) {

    if (lexOperatorToken('-')) {
        auto start = tokens.size() - 1;
        if (!(lexParenExpression() || lexAccessChainOrValue(false))) {
            error("expected an expression after '-' negative");
        }
        compound_from(start, LexTokenType::CompNegative);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken('!')) {
        auto start = tokens.size() - 1;
        if (!(lexParenExpression() || lexAccessChainOrValue(false))) {
            error("expected an expression after '!' not");
        }
        compound_from(start, LexTokenType::CompNot);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken('(')) {
        unsigned start = tokens.size() - 1;
        if (lambda && lexLambdaOrExprAfterLParen()) {
            return true;
        }
        lexParenExpressionAfterLParen();
        if(!lexRemainingExpression(start)) {
            compound_from(start, LexTokenType::CompExpression);
        }
        return true;
    }

    if (!lexAccessChainOrValue(lexStruct)) {
        return false;
    } else if(lexStruct && tokens[tokens.size() - 1]->is_struct_value()) {
        return true;
    }

    lexWhitespaceToken();

    if (provider.peek() == '<' && isGenericEndAhead()) {
        auto start = tokens.size() - 1;
        lexFunctionCallWithGenericArgsList();
        if(lexOperatorToken('.') && !lexAccessChainRecursive(false)) {
            error("expected a identifier after the dot . in the access chain");
            return true;
        }
        compound_from(start, LexTokenType::CompAccessChain);
        return true;
    }

    lexRemainingExpression(tokens.size() - 1);

    return true;

}