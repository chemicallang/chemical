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
    unsigned index = 0; // param identifier index
    do {
        lexWhitespaceToken();
        if(index == 0) {
            if(lexOperatorToken('&')) {
                if(lexIdentifierToken()) {
                    compound_from<FunctionParamCST>(tokens.size() - 2);
                    lexWhitespaceToken();
                    index++;
                    continue;
                } else {
                    error("expected a identifier right after '&' in the first function parameter as a 'self' parameter");
                    break;
                }
            }
        }
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
            index++;
        }
        lexWhitespaceToken();
    } while(lexOperatorToken(','));
}

bool Lexer::lexAfterFuncKeyword(bool allow_extensions) {

    lexWhitespaceToken();

    if(allow_extensions) {
        if(lexOperatorToken('(')) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                error("expected identifier for receiver in extension function after '('");
                return false;
            }
            lexWhitespaceToken();
            if(!lexOperatorToken(':')) {
                error("expected ':' in extension function after identifier for receiver");
                return false;
            }
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected type after ':' in extension function for receiver");
                return false;
            }
            lexWhitespaceToken();
            if(!lexOperatorToken(')')) {
                error("expected ')' in extension function after receiver");
                return false;
            }
            lexWhitespaceToken();
        }
    }

    if(!lexIdentifierToken()) {
        error("function name is missing after the keyword 'func'");
        return false;
    }

    lexWhitespaceToken();

    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a function signature");
        return false;
    }

    lexParameterList();

    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) when ending a function signature");
        return false;
    }

    lexWhitespaceToken();

    if(lexOperatorToken(':')) {
        lexWhitespaceToken();
        if(!lexTypeTokens()) {
            error("expected a return type for function after ':'");
            return false;
        }
    }

    return true;

}

bool Lexer::lexFunctionSignatureTokens() {

    if(!lexKeywordToken("func")) {
        return false;
    }

    return lexAfterFuncKeyword();

}

bool Lexer::lexFunctionStructureTokens(bool allow_declarations, bool allow_extensions) {

    if(!lexKeywordToken("func")) {
        return false;
    }

    unsigned start = tokens.size() - 1;

    if(!lexAfterFuncKeyword(allow_extensions)) {
        return true;
    }

    // inside the block allow return statements
    auto prevReturn = isLexReturnStatement;
    isLexReturnStatement = true;
    if(!lexBraceBlock("function") && !allow_declarations) {
        error("expected the function definition after the signature");
    }
    isLexReturnStatement = prevReturn;

    compound_collectable<FunctionCST>(start);

    return true;

}