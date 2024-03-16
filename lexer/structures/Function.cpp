// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/FunctionToken.h"
#include "lexer/model/tokens/ParameterToken.h"

bool Lexer::lexReturnStatement() {
    if(lexKeywordToken("return")) {
        lexWhitespaceToken();
        lexExpressionTokens();
        return true;
    } else {
        return false;
    }
}

void Lexer::lexParameterList() {
    do {
        lexWhitespaceToken();
        auto name = lexAlpha();
        if(!name.empty()) {
            tokens.emplace_back(std::make_unique<ParameterToken>(backPosition(name.length()), name));
            lexWhitespaceToken();
            if(!lexOperatorToken(':')) {
                error("missing a type for the function parameter, expected a colon after the name");
                return;
            }
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("missing a type token for the function parameter, expected type after the colon");
                return;
            }
            if(lexOperatorToken("...")) {
                break;
            }
        }
        lexWhitespaceToken();
    } while(lexOperatorToken(','));
}

bool Lexer::lexFunctionSignatureTokens() {

    if(!lexKeywordToken("func")) {
        return false;
    }

    lexWhitespaceToken();

    auto name = lexAlpha();
    if(name.empty()) {
        error("function name is missing, when defining a function");
        return true;
    } else {
#ifdef LSP_BUILD
        tokens.emplace_back(std::make_unique<FunctionToken>(backPosition(name.length()), name, modifiers));
        modifiers = 0;
#else
        tokens.emplace_back(std::make_unique<FunctionToken>(backPosition(name.length()), name));
#endif
    }

    lexWhitespaceToken();

    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a function signature");
        return true;
    }

    lexParameterList();

    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) when ending a function signature");
        return true;
    }

    lexWhitespaceToken();

    if(lexOperatorToken(':')) {
        lexWhitespaceToken();
        lexTypeTokens();
    }

    return true;

}

bool Lexer::lexFunctionStructureTokens(bool allow_declarations) {

    if(!lexFunctionSignatureTokens()) {
        return false;
    }

    // inside the block allow return statements
    auto prevReturn = isLexReturnStatement;
    isLexReturnStatement = true;
    if(!lexBraceBlock("function") && !allow_declarations) {
        error("expected the function definition after the signature");
    }
    isLexReturnStatement = prevReturn;

    return true;

}