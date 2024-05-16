// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/ImplCST.h"

void Lexer::lexImplBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens())) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
}

bool Lexer::lexImplTokens() {
    if (lexKeywordToken("impl")) {
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexVariableToken()) {
            error("expected interface name after the interface keyword in implementation");
            return true;
        }
        lexWhitespaceToken();
        if(lexKeywordToken("for")) {
            lexWhitespaceToken();
            if(!lexVariableToken()) {
                error("expected a struct name after the 'for' keyword in implementation");
                return true;
            }
            lexWhitespaceToken();
        }
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an implementation");
            return true;
        }
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