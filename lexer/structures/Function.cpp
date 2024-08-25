// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexReturnStatement() {
    if(lexWSKeywordToken("return", ';')) {
        unsigned int start = tokens_size() - 1;
        lexWhitespaceToken();
        lexExpressionTokens(true);
        compound_from(start, LexTokenType::CompReturn);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexDestructStatement() {
    if(lexWSKeywordToken("destruct", '[')) {
        unsigned start = tokens_size() - 1;
        if(lexOperatorToken('[')) {
            lexWhitespaceToken();
            // optional value
            lexAccessChainOrValue();
            lexWhitespaceToken();
            if(!lexOperatorToken(']')) {
                error("expected a ']' after the access chain value");
                return true;
            }
            if(!lexWhitespaceToken()) {
                error("expected whitespace after ']' in destruct statement");
                return true;
            }
        }
        if(!lexAccessChainOrValue()) {
            error("expected a pointer value for the destruct statement");
            return true;
        }
        compound_from(start, LexTokenType::CompDestruct);
        return true;
    } else {
        return false;
    }
}

void Lexer::lexParameterList(bool optionalTypes, bool defValues, bool lexSelfParam, bool variadicParam) {
    unsigned index = 0; // param identifier index
    do {
        lexWhitespaceAndNewLines();
        if(lexSelfParam && index == 0 && lexOperatorToken('&')) {
            if(lexIdentifierToken()) {
                compound_from(tokens_size() - 2, LexTokenType::CompFunctionParam);
                lexWhitespaceToken();
                index++;
                continue;
            } else {
                error("expected a identifier right after '&' in the first function parameter as a 'self' parameter");
                break;
            }
        }
        if(lexIdentifierToken()) {
            unsigned int start = tokens_size() - 1;
            lexWhitespaceToken();
            if(lexOperatorToken(':')) {
                lexWhitespaceToken();
                if(lexTypeTokens()) {
                    if(variadicParam && lexOperatorToken("...")) {
                        compound_from(start, LexTokenType::CompFunctionParam);
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
                    compound_from(start, LexTokenType::CompFunctionParam);
                } else {
                    error("missing a type token for the function parameter, expected type after the colon");
                    return;
                }
            } else {
                if(optionalTypes) {
                    compound_from(start, LexTokenType::CompFunctionParam);
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

bool Lexer::lexGenericParametersList() {
    if(lexOperatorToken('<')) {
        unsigned start = tokens_size() - 1;
        while(true) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                break;
            }
            lexWhitespaceToken();
            if(lexOperatorToken('=')) {
                lexWhitespaceToken();
                if(!lexTypeTokens()) {
                    error("expected a default type after '=' in generic parameter list");
                    return true;
                }
            }
            lexWhitespaceToken();
            if(!lexOperatorToken(',')) {
                break;
            }
        }
        if(!lexOperatorToken('>')) {
            error("expected a '>' for ending the generic parameters list");
            return true;
        }
        compound_from(start, LexTokenType::CompGenericParamsList);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAfterFuncKeyword(bool allow_extensions) {

    if(lexGenericParametersList() && has_errors) {
        return false;
    }

    lexWhitespaceToken();

    if(allow_extensions && lexOperatorToken('(')) {
        lexWhitespaceToken();
        unsigned start = tokens_size();
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
        compound_from(start, LexTokenType::CompFunctionParam);
        lexWhitespaceToken();
        if(!lexOperatorToken(')')) {
            error("expected ')' in extension function after receiver");
            return false;
        }
        lexWhitespaceToken();
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

    lexWhitespaceAndNewLines();

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

    if(!lexWSKeywordToken("func")) {
        return false;
    }

    return lexAfterFuncKeyword();

}

bool Lexer::lexFunctionStructureTokens(bool allow_declarations, bool allow_extensions) {

    if(!lexWSKeywordToken("func")) {
        return false;
    }

    unsigned start = tokens_size() - 1;

    if(!lexAfterFuncKeyword(allow_extensions)) {
        return true;
    }

    // inside the block allow return statements
    auto prevReturn = isLexReturnStatement;
    isLexReturnStatement = true;
    auto prevDestruct = isLexDestructStatement;
    isLexDestructStatement = true;
    if(!lexBraceBlock("function") && !allow_declarations) {
        error("expected the function definition after the signature");
    }
    isLexReturnStatement = prevReturn;
    isLexDestructStatement = prevDestruct;

    compound_collectable(start, LexTokenType::CompFunction);

    return true;

}