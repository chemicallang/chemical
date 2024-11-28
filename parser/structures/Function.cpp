// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexReturnStatement() {
    if(lexWSKeywordToken(TokenType::ReturnKw, TokenType::SemiColonSym)) {
        unsigned int start = tokens_size() - 1;
        lexWhitespaceToken();
        lexExpressionTokens(true);
        compound_from(start, LexTokenType::CompReturn);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexConstructorInitBlock() {
    if(lexWSKeywordToken(TokenType::InitKw, TokenType::LBrace)) {
        unsigned start = tokens_size() - 1;
        if(!lexOperatorToken(TokenType::LBrace)) {
            error("expected a '{' after init for init block");
            return true;
        }
        while(true) {
            lexWhitespaceAndNewLines();
            if(lexAccessChain(false, true)) {
                lexOperatorToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        }
        if(!lexOperatorToken(TokenType::RBrace)) {
            error("expected a '}' for ending the init block");
            return true;
        }
        compound_from(start + 1, LexTokenType::CompBody);
        compound_from(start, LexTokenType::CompInitBlock);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexUnsafeBlock() {
    if(lexWSKeywordToken(TokenType::UnsafeKw, TokenType::LBrace)) {
        unsigned start = tokens_size() - 1;
        if(!lexBraceBlock("unsafe_block")) {
            error("expected a braced block after 'unsafe' keyword");
            return true;
        }
        compound_from(start, LexTokenType::CompUnsafeBlock);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexDestructStatement() {
    if(lexWSKeywordToken(TokenType::DestructKw, TokenType::LBracket)) {
        unsigned start = tokens_size() - 1;
        if(lexOperatorToken(TokenType::LBracket)) {
            lexWhitespaceToken();
            // optional value
            lexAccessChainOrValue();
            lexWhitespaceToken();
            if(!lexOperatorToken(TokenType::RBracket)) {
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

bool Parser::lexParameterList(bool optionalTypes, bool defValues, bool lexImplicitParams, bool variadicParam) {
    do {
        lexWhitespaceAndNewLines();
        if(lexImplicitParams && lexOperatorToken(TokenType::AmpersandSym)) {
            const auto start = tokens_size() - 1;
            lexWSKeywordToken(TokenType::MutKw); // optional mut keyword
            if(lexIdentifierToken()) {
                compound_from(start, LexTokenType::CompFunctionParam);
                lexWhitespaceToken();
                continue;
            } else {
                error("expected a identifier right after '&' in the first function parameter as a 'self' parameter");
                return false;
            }
        }
        if(lexIdentifierToken()) {
            unsigned int start = tokens_size() - 1;
            lexWhitespaceToken();
            if(lexOperatorToken(TokenType::ColonSym)) {
                lexWhitespaceToken();
                if(lexTypeTokens()) {
                    if(variadicParam && lexOperatorToken(TokenType::TripleDotSym)) {
                        compound_from(start, LexTokenType::CompFunctionParam);
                        break;
                    }
                    if(defValues) {
                        lexWhitespaceToken();
                        if (lexOperatorToken(TokenType::EqualSym)) {
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
                    return false;
                }
            } else {
                if(optionalTypes) {
                    compound_from(start, LexTokenType::CompFunctionParam);
                } else {
                    error("expected colon ':' in function parameter list after the parameter name ");
                    return false;
                }
            }
        }
        lexWhitespaceToken();
    } while(lexOperatorToken(TokenType::CommaSym));
    return true;
}

bool Parser::lexGenericParametersList() {
    if(lexOperatorToken(TokenType::LessThanSym)) {
        unsigned start = tokens_size() - 1;
        while(true) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                break;
            }
            lexWhitespaceToken();
            if(lexOperatorToken(TokenType::ColonSym)) {
                lexWhitespaceToken();
                if(!lexTypeTokens()) {
                    error("expected a type after ':' in generic parameter list");
                    return true;
                }
                lexWhitespaceToken();
            }
            if(lexOperatorToken(TokenType::EqualSym)) {
                lexWhitespaceToken();
                if(!lexTypeTokens()) {
                    error("expected a default type after '=' in generic parameter list");
                    return true;
                }
                lexWhitespaceToken();
            }
            if(!lexOperatorToken(TokenType::CommaSym)) {
                break;
            }
        }
        if(!lexOperatorToken(TokenType::GreaterThanSym)) {
            error("expected a '>' for ending the generic parameters list");
            return true;
        }
        compound_from(start, LexTokenType::CompGenericParamsList);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexAfterFuncKeyword(bool allow_extensions) {

    if(lexGenericParametersList() && has_errors) {
        return false;
    }

    lexWhitespaceToken();

    if(allow_extensions && lexOperatorToken(TokenType::LParen)) {
        lexWhitespaceToken();
        unsigned start = tokens_size();
        if(!lexIdentifierToken()) {
            error("expected identifier for receiver in extension function after '('");
            return false;
        }
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::ColonSym)) {
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
        if(!lexOperatorToken(TokenType::RParen)) {
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

    if(!lexOperatorToken(TokenType::LParen)) {
        error("expected a starting parenthesis ( in a function signature");
        return false;
    }

    if(!lexParameterList()) {
        return false;
    }

    lexWhitespaceAndNewLines();

    if(!lexOperatorToken(TokenType::RParen)) {
        error("expected a closing parenthesis ) when ending a function signature");
        return false;
    }

    lexWhitespaceToken();

    if(lexOperatorToken(TokenType::ColonSym)) {
        lexWhitespaceToken();
        if(!lexTypeTokens()) {
            error("expected a return type for function after ':'");
            return true; // return true, since type is not found, but we continue lexing other function
        }
    }

    return true;

}

bool Parser::lexFunctionSignatureTokens() {

    if(!lexWSKeywordToken(TokenType::FuncKw)) {
        return false;
    }

    return lexAfterFuncKeyword();

}

bool Parser::lexFunctionStructureTokens(unsigned start, bool allow_declarations, bool allow_extensions) {

    if(!lexWSKeywordToken(TokenType::FuncKw)) {
        return false;
    }

    if(!lexAfterFuncKeyword(allow_extensions)) {
        return true;
    }

    // inside the block allow return statements
    auto prevReturn = isLexReturnStatement;
    isLexReturnStatement = true;
    isLexInitBlock = true;
    if(!lexBraceBlock("function") && !allow_declarations) {
        error("expected the function definition after the signature");
    }
    isLexInitBlock = false;
    isLexReturnStatement = prevReturn;

    compound_from(start, LexTokenType::CompFunction);

    return true;

}