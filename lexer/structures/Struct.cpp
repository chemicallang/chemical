// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/StructDefCST.h"

bool Lexer::lexStructMemberTokens() {
    return lexVarInitializationTokens(true, true) || lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens();
}

bool Lexer::lexStructBlockTokens() {
    if(lexOperatorToken('{')) {
        do {
            lexWhitespaceAndNewLines();
            if(!lexStructMemberTokens()) {
                break;
            }
            lexWhitespaceToken();
            lexOperatorToken(';');
        } while(provider.peek() != '}');
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
            return true;
        }
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                error("expected a interface name after ':' when declaring a struct");
                return true;
            }
        }
        lexWhitespaceToken();
        if(!lexStructBlockTokens()) {
            error("expected a struct block for declaring struct members");
            return true;
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