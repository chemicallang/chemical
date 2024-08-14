// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"

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
        lexGenericParametersList();
        lexWhitespaceToken();
        if(!lexRefOrGenericType()) return true;
        lexWhitespaceToken();
        if(lexKeywordToken("for")) {
            lexWhitespaceToken();
            if(!lexRefOrGenericType()) return true;
            lexWhitespaceToken();
        }
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an implementation");
            return true;
        }
        auto prev = isCBICollecting;
        isCBICollecting = false;
        lexImplBlockTokens();
        isCBICollecting = prev;
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an implementation");
            return true;
        }
        compound_collectable(start, LexTokenType::CompImpl);
        return true;
    }
    return false;
}