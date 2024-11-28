// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexVarInitializationTokens(unsigned start, bool allowDeclarations, bool requiredType) {

    auto lexed_const = lexWSKeywordToken(TokenType::ConstKw);

    if (!lexed_const && !lexWSKeywordToken(TokenType::VarKw)) {
        return false;
    }

    // identifier
    if (!lexIdentifierToken()) {
        error("expected an identifier for variable initialization");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    bool has_type = false;

    // :
    if (lexOperatorToken(TokenType::ColonSym)) {

        // whitespace
        lexWhitespaceToken();

        // type
        if(lexTypeTokens()) {
            has_type = true;
        } else if(requiredType) {
            error("expected type tokens for variable initialization");
            return true;
        }

        // whitespace
        lexWhitespaceToken();

    } else if(requiredType) {
        error("expected ':' for type");
        return true;
    }

    // equal sign
    if (!lexOperatorToken(TokenType::EqualSym)) {
        if(!allowDeclarations) {
            error("expected an = sign for the initialization of the variable");
            return true;
        } else if(has_type) {
            compound_from(start, LexTokenType::CompVarInit);
        } else {
            error("a type or value is required to initialize a variable");
            return true;
        }
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!(lexExpressionTokens(true) || lexArrayInit())) {
        error("expected an expression / array for variable initialization");
        return true;
    }

    compound_from(start, LexTokenType::CompVarInit);

    return true;

}