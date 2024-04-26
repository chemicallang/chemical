// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/ImplCST.h"

void Lexer::lexImplBlockTokens() {
    do {
        lexWhitespaceToken();
        lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens();
        lexWhitespaceToken();
        lexOperatorToken(';');
        lexWhitespaceToken();
        lexNewLineChars();
    } while(provider.peek() != '}');
}

bool Lexer::lexImplTokens() {
    if (lexKeywordToken("impl")) {
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected interface name after the interface keyword in implementation");
            return true;
        }
        lexWhitespaceToken();
        if(lexKeywordToken("for")) {
            lexWhitespaceToken();
            if(!lexIdentifierToken()) {
                error("expected a struct name after the 'for' keyword in implementation");
                return true;
            }
            lexWhitespaceToken();
        }
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an implementation");
            return true;
        }
        lexWhitespaceToken();
        lexNewLineChars();
        lexImplBlockTokens();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an implementation");
            return true;
        }
        compound_from<ImplCST>(start);
        return true;
    }
    return false;
}