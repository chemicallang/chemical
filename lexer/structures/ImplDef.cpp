// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexImplBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens() || lexAnnotationMacro())) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
}

bool Lexer::lexImplTokens() {
    if (lexWSKeywordToken("impl")) {
        auto start = tokens_size() - 1;
        lexGenericParametersList();
        lexWhitespaceToken();
        if(!lexRefOrGenericType()) return true;
        lexWhitespaceToken();
        if(lexWSKeywordToken("for")) {
            if(!lexRefOrGenericType()) return true;
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
        compound_from(start, LexTokenType::CompImpl);
        return true;
    }
    return false;
}