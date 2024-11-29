// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexEnumBlockTokens() {
    while(true) {
        lexWhitespaceAndNewLines();
        if(lexIdentifierToken()) {
            lexWhitespaceToken();
            if(lexOperatorToken(TokenType::EqualSym)) {
                lexWhitespaceToken();
                if(!lexExpressionTokens()) {
                    error("expected a value after '=' operator");
                    return false;
                }
            }
            if(lexOperatorToken(TokenType::CommaSym)) {
                continue;
            } else {
                lexWhitespaceAndNewLines();
                break;
            }
        } else if(lexSingleLineCommentTokens() || lexMultiLineCommentTokens()) {
            continue;
        } else {
            break;
        }
    };
    return true;
}

bool Parser::lexEnumStructureTokens(unsigned start) {
    if(lexWSKeywordToken(TokenType::EnumKw)) {
        if(!lexIdentifierToken()) {
            error("expected a identifier as enum name");
            return true;
        }
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::LBrace)) {
            error("expected a '{' for after the enum name");
            return true;
        }
        if(!lexEnumBlockTokens()) {
            return true;
        }
        if(!lexOperatorToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' in enum block");
            return true;
        }
        compound_from(start, LexTokenType::CompEnumDecl);
        return true;
    } else {
        return false;
    }
}