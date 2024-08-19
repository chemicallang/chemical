// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexEnumBlockTokens() {
    if(lexOperatorToken('{')) {
        while(true) {
            lexWhitespaceAndNewLines();
            if(lexIdentifierToken()) {
                lexWhitespaceToken();
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
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' in enum block");
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexEnumStructureTokens() {
    if(lexWSKeywordToken("enum")) {
        auto start = tokens.size() - 1;
        if(!lexIdentifierToken()) {
            error("expected a identifier as enum name");
            return true;
        }
        lexWhitespaceToken();
        if(!lexEnumBlockTokens()) {
            error("expected an enum block for declaring an enum");
            return true;
        }
        compound_collectable(start, LexTokenType::CompEnumDecl);
        return true;
    } else {
        return false;
    }
}