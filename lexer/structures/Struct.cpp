// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/StructDefCST.h"

bool Lexer::lexStructMemberTokens() {
    return lexVarInitializationTokens(true, true) || lexFunctionStructureTokens();
}

bool Lexer::lexStructBlockTokens() {
    if(lexOperatorToken('{')) {
        lexWhitespaceToken();
        lexNewLineChars();
        do {
            lexWhitespaceToken();
            lexStructMemberTokens();
            lexWhitespaceToken();
            lexOperatorToken(';');
            lexWhitespaceToken();
        }while(lexNewLineChars());
        lexWhitespaceToken();
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' in enum block");
        }
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexStructStructureTokens() {
    if(lexKeywordToken("struct")) {
        auto start_token = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected a identifier as struct name");
        }
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                error("expected a interface name after ':' when declaring a struct");
            }
        }
        lexWhitespaceToken();
        if(!lexStructBlockTokens()) {
            error("expected a struct block for declaring struct members");
        }
        lexWhitespaceToken();
        compound_from<StructDefCST>(start_token);
        if(isLexerScoped) {
            collectStructAsLexer(start_token, tokens.size());
            isLexerScoped = false;
        }
        return true;
    } else {
        return false;
    }
}