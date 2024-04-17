// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/values/NegativeCST.h"
#include "cst/values/NotCST.h"
#include "cst/values/CastCST.h"
#include "cst/values/ExpressionCST.h"

void Lexer::lexRemainingExpression(unsigned start) {

    lexWhitespaceToken();
    if(lexKeywordToken("as")) {
        lexWhitespaceToken();
        if(!lexTypeTokens()) {
            error("expected a type for casting after 'as' in expression");
            return;
        }
        compound_from<CastCST>(start);
        return;
    }
    if(!lexLanguageOperatorToken()) {
        return;
    }
    lexWhitespaceToken();
    if(!lexExpressionTokens()) {
        error("expected an expression after the operator token in the expression");
        return;
    }

    compound_from<ExpressionCST>(start);

}

bool Lexer::lexExpressionTokens(bool lexStruct){

    if(lexOperatorToken('-')) {
        auto start = tokens.size() - 1;
        if(!lexExpressionTokens()) {
            error("expected a expression after '-' negative");
        }
        compound_from<NegativeCST>(start);
        return true;
    }

    if(lexOperatorToken('!')) {
        auto start = tokens.size() - 1;
        if(!lexExpressionTokens()) {
            error("expected a expression '!' for not");
        }
        compound_from<NotCST>(start);
        return true;
    }

    if(lexOperatorToken('(')) {

        unsigned start = tokens.size() - 1;

        if(!lexExpressionTokens()) {
            error("expected a nested expression after starting parenthesis ( in the expression");
            return true;
        };

        if(!lexOperatorToken(')')) {
            error("missing ) in the expression");
            return true;
        }

        lexRemainingExpression(start);

        return true;

    }

    if(!lexAccessChainOrValue(lexStruct)) {
        return false;
    }

    lexRemainingExpression(tokens.size() - 1);

    return true;

}