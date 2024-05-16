// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/values/NegativeCST.h"
#include "cst/values/NotCST.h"
#include "cst/values/CastCST.h"
#include "cst/values/ExpressionCST.h"
#include "cst/structures/FunctionCST.h"
#include "cst/values/AccessChainCST.h"

void Lexer::lexRemainingExpression(unsigned start) {

    lexWhitespaceToken();
    if (lexKeywordToken("as")) {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            error("expected a type for casting after 'as' in expression");
            return;
        }
        compound_from<CastCST>(start);
        return;
    }
    if (!lexLanguageOperatorToken()) {
        return;
    }
    lexWhitespaceToken();
    if (!lexExpressionTokens(false, false)) {
        error("expected an expression after the operator token in the expression");
        return;
    }

    compound_from<ExpressionCST>(start);

}

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

bool Lexer::lexLambdaAfterLParen() {
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

    if (lexOperatorToken(')')) {
        compound_range<FunctionParamCST>(start, tokens.size() - 1);
        lexLambdaAfterParamsList(start);
        return true;
    } else if (lexOperatorToken(':')) {
        lexWhitespaceToken();
        if (lexTypeTokens()) {
            lexWhitespaceToken();
        } else {
            error("expected a type after ':' when lexing a lambda in parenthesized expression");
        }
        compound_from<FunctionParamCST>(start);
        if (lexOperatorToken(',')) {
            lexParameterList(true, false);
        }
        lexLambdaAfterComma(this, start);
        return true;
    } else if (lexOperatorToken(',')) {
        lexParameterList(true, false);
        lexLambdaAfterComma(this, start);
        return true;
    }

    if(has_whitespace) {
        lexRemainingExpression(start);
        if(!lexOperatorToken(')')) {
            error("expected ')' after the nested parenthesized expression");
        }
        return true;
    } else {
        lexAccessChainAfterId();
        if(!tokens[start + 1]->is_struct_value()) {
            compound_from<AccessChainCST>(start + 1);
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
        compound_from<NegativeCST>(start);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken('!')) {
        auto start = tokens.size() - 1;
        if (!(lexParenExpression() || lexAccessChainOrValue(false))) {
            error("expected an expression after '!' not");
        }
        compound_from<NotCST>(start);
        lexRemainingExpression(start);
        return true;
    }

    if (lexOperatorToken('(')) {
        unsigned start = tokens.size() - 1;
        if (lambda && lexLambdaAfterLParen()) {
            return true;
        }
        lexParenExpressionAfterLParen();
        lexRemainingExpression(start);
        return true;
    }

    if (!lexAccessChainOrValue(lexStruct)) {
        return false;
    }

    lexRemainingExpression(tokens.size() - 1);

    return true;

}