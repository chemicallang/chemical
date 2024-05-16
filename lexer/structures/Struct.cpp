// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/StructDefCST.h"

bool Lexer::lexStructMemberTokens() {
    return lexVarInitializationTokens(true, true) || lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens();
}

void Lexer::lexStructBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!lexStructMemberTokens()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Lexer::lexStructStructureTokens() {
    if(lexKeywordToken("struct")) {
        auto start_token = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexVariableToken()) {
            error("expected a identifier as struct name");
            return true;
        }
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            lexWhitespaceToken();
            if(!lexVariableToken()) {
                error("expected a interface name after ':' when declaring a struct");
                return true;
            }
        }
        lexWhitespaceToken();
        if(!lexOperatorToken('{')) {
            error("expected a '{' for struct block");
            return true;
        }
        lexStructBlockTokens();
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for struct block");
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