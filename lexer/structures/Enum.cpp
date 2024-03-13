// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/EnumToken.h"
#include "lexer/model/tokens/EnumMemberToken.h"

bool Lexer::lexEnumBlockTokens() {
    if(lexOperatorToken('{')) {
        do {
            lexWhitespaceToken();
            lexNewLineChars();
            lexWhitespaceToken();
            auto id = lexIdentifier();
            if(id.empty()) {
                error("expected a identifier as enum member");
            } else {
                tokens.emplace_back(std::make_unique<EnumMemberToken>(backPosition(id.length()), id));
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(','));
        lexNewLineChars();
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' in enum block");
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexEnumStructureTokens() {
    if(lexKeywordToken("enum")) {
        lexWhitespaceToken();
        auto id = lexIdentifier();
        if(id.empty()) {
            error("expected a identifier as enum name");
        } else {
            tokens.emplace_back(std::make_unique<EnumToken>(backPosition(id.length()), id));
        }
        lexWhitespaceToken();
        if(!lexEnumBlockTokens()) {
            error("expected an enum block for declaring an enum");
        }
        lexWhitespaceToken();
        return true;
    } else {
        return false;
    }
}