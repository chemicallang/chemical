// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Lexer.h"

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

bool Lexer::lexInterfaceStructureTokens(unsigned start) {
    if (lexWSKeywordToken("interface")) {
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
        lexInterfaceBlockTokens();
        lexWhitespaceToken();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface block");
            return true;
        }
        compound_from(start, LexTokenType::CompInterface);
        return true;
    }
    return false;
}