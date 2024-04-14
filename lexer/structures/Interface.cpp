// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexInterfaceBlockTokens() {
    do {
        lexWhitespaceToken();
        lexFunctionSignatureTokens() || lexVarInitializationTokens(true);
        lexWhitespaceToken();
        lexOperatorToken(';');
        lexWhitespaceToken();
    } while (lexNewLineChars());
}

bool Lexer::lexInterfaceStructureTokens() {
    if (lexKeywordToken("interface")) {
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected interface name after the interface keyword");
            return true;
        }
        lexWhitespaceToken();
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an interface");
            return true;
        }
        lexWhitespaceToken();
        lexNewLineChars();
        lexInterfaceBlockTokens();
        lexWhitespaceToken();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface");
            return true;
        }
        return true;
    }
    return false;
}