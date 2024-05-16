// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/EnumDeclCST.h"

bool Lexer::lexEnumBlockTokens() {
    if(lexOperatorToken('{')) {
        do {
            lexWhitespaceToken();
            lexNewLineChars();
            lexWhitespaceToken();
            if(!lexVariableToken()) {
                break;
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
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexVariableToken()) {
            error("expected a identifier as enum name");
            return true;
        }
        lexWhitespaceToken();
        if(!lexEnumBlockTokens()) {
            error("expected an enum block for declaring an enum");
            return true;
        }
        compound_from<EnumDeclCST>(start);
        return true;
    } else {
        return false;
    }
}