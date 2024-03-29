// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/StructToken.h"
#include "lexer/model/tokens/InterfaceToken.h"

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
        auto id = provider.readIdentifier();
        if(id.empty()) {
            error("expected a identifier as struct name");
        } else {
            tokens.emplace_back(std::make_unique<StructToken>(backPosition(id.length()), id));
        }
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            lexWhitespaceToken();
            auto inter = provider.readIdentifier();
            if(!inter.empty()) {
                tokens.emplace_back(std::make_unique<InterfaceToken>(backPosition(inter.length()), inter));
            } else {
                error("expected a interface name after ':' when declaring a struct");
            }
        }
        lexWhitespaceToken();
        if(!lexStructBlockTokens()) {
            error("expected a struct block for declaring struct members");
        }
        lexWhitespaceToken();
        if(isLexerScoped) {
            collectStructAsLexer(start_token, tokens.size());
            isLexerScoped = false;
        }
        return true;
    } else {
        return false;
    }
}