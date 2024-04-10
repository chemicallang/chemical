// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/values/NegativeCST.h"
#include "cst/values/NotCST.h"
#include "cst/values/CastCST.h"

void Lexer::lexRemainingExpression() {
    lexWhitespaceToken();
    if(lexKeywordToken("as")) {
        lexWhitespaceToken();
        if(!lexTypeTokens()) {
            error("expected a type for casting after 'as' in expression");
        }
        compound<CastCST>();
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

        if(!lexExpressionTokens()) {
            error("expected a nested expression after starting parenthesis ( in the expression");
            return true;
        };

        if(!lexOperatorToken(')')) {
            error("missing ) in the expression");
            return true;
        }

        lexRemainingExpression();

        return true;

    }

    if(!lexAccessChainOrValue(lexStruct)) {
        return false;
    }

    lexRemainingExpression();

    return true;

}