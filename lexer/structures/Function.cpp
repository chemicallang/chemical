// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/FunctionCST.h"
#include "cst/statements/ReturnCST.h"

bool Lexer::lexReturnStatement() {
    if(lexKeywordToken("return")) {
        unsigned int start = tokens.size() - 1;
        lexWhitespaceToken();
        lexExpressionTokens(true);
        compound_from<ReturnCST>(start);
        return true;
    } else {
        return false;
    }
}

void Lexer::lexParameterList(bool optionalTypes, bool defValues) {
    nested_compound_start();
    do {
        lexWhitespaceToken();
        if(lexIdentifierToken()) {
            unsigned int start = tokens.size() - 1;
            lexWhitespaceToken();
            if(lexOperatorToken(':')) {
                lexWhitespaceToken();
                if(lexTypeTokens()) {
                    if(lexOperatorToken("...")) {
                        compound_from<FunctionParamCST>(start);
                        break;
                    }
                    if(defValues) {
                        lexWhitespaceToken();
                        if (lexOperatorToken('=')) {
                            lexWhitespaceToken();
                            if (!lexValueToken()) {
                                error("expected value after '=' for default value for the parameter");
                                break;
                            }
                        }
                    }
                    compound_from<FunctionParamCST>(start);
                } else {
                    error("missing a type token for the function parameter, expected type after the colon");
                    return;
                }
            } else {
                if(optionalTypes) {
                    compound_from<FunctionParamCST>(start);
                } else {
                    error("expected colon ':' in function parameter list after the parameter name ");
                    return;
                }
            }
        }
        lexWhitespaceToken();
    } while(lexOperatorToken(','));
    nested_compound_end();
}

bool Lexer::lexAfterFuncKeyword() {

    lexWhitespaceToken();

    if(!lexIdentifierToken()) {
        error("function name is missing after the keyword 'func'");
        return true;
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

bool Lexer::lexFunctionSignatureTokens() {

    if(!lexKeywordToken("func")) {
        return false;
    }

    return lexAfterFuncKeyword();

}

bool Lexer::lexFunctionStructureTokens(bool allow_declarations) {

    if(!lexKeywordToken("func")) {
        return false;
    }

    unsigned start = tokens.size() - 1;

    lexAfterFuncKeyword();

    // inside the block allow return statements
    auto prevReturn = isLexReturnStatement;
    isLexReturnStatement = true;
    if(!lexBraceBlock("function") && !allow_declarations) {
        error("expected the function definition after the signature");
    }
    isLexReturnStatement = prevReturn;

    compound_from<FunctionCST>(start);

    return true;

}