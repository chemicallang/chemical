// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexEnumBlockTokens() {
    while(true) {
        lexWhitespaceAndNewLines();
        if(lexIdentifierToken()) {
            lexWhitespaceToken();
            if(lexOperatorToken('=') && !lexConstantValue()) {
                error("expected a value after '=' operator");
                return false;
            }
            if(lexOperatorToken(',')) {
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

bool Lexer::lexEnumStructureTokens(unsigned start) {
    if(lexWSKeywordToken("enum")) {
        if(!lexIdentifierToken()) {
            error("expected a identifier as enum name");
            return true;
        }
        lexWhitespaceToken();
        if(!lexOperatorToken('{')) {
            error("expected a '{' for after the enum name");
            return true;
        }
        if(!lexEnumBlockTokens()) {
            return true;
        }
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' in enum block");
            return true;
        }
        compound_from(start, LexTokenType::CompEnumDecl);
        return true;
    } else {
        return false;
    }
}