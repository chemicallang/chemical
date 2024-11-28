// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

void Parser::lexInterfaceBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(lexVarInitializationTokens(true, true) || lexFunctionStructureTokens(true) || lexSingleLineCommentTokens() || lexMultiLineCommentTokens())) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    } while(token->type != TokenType::RBrace);
}

bool Parser::lexInterfaceStructureTokens(unsigned start) {
    if (lexWSKeywordToken(TokenType::InterfaceKw)) {
        if(!lexIdentifierToken()) {
            error("expected interface name after the interface keyword");
            return true;
        }
        lexWhitespaceToken();
        lexGenericParametersList();
        lexWhitespaceToken();
        if (!lexOperatorToken(TokenType::LBrace)) {
            error("expected a '{' when starting an interface block");
            return true;
        }
        lexInterfaceBlockTokens();
        lexWhitespaceToken();
        if (!lexOperatorToken(TokenType::RBrace)) {
            error("expected a '}' when ending an interface block");
            return true;
        }
        compound_from(start, LexTokenType::CompInterface);
        return true;
    }
    return false;
}