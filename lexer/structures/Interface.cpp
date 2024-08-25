// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexInterfaceBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(lexVarInitializationTokens(true, true) || lexFunctionStructureTokens(true) || lexSingleLineCommentTokens() || lexMultiLineCommentTokens())) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
}

bool Lexer::lexInterfaceStructureTokens() {
    if (lexWSKeywordToken("interface")) {
        unsigned start = tokens_size() - 1;
        if(!lexIdentifierToken()) {
            error("expected interface name after the interface keyword");
            return true;
        }
        lexWhitespaceToken();
        lexGenericParametersList();
        lexWhitespaceToken();
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an interface block");
            return true;
        }
        auto prev = isCBICollecting;
        isCBICollecting = false;
        lexInterfaceBlockTokens();
        isCBICollecting = prev;
        lexWhitespaceToken();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface block");
            return true;
        }
        compound_collectable(start, LexTokenType::CompInterface);
        return true;
    }
    return false;
}