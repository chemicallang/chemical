// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/InterfaceCST.h"

void Lexer::lexInterfaceBlockTokens() {
    lexWhitespaceToken();
    lexNewLineChars();
    do {
        lexWhitespaceToken();
        lexVarInitializationTokens(true, true) || lexFunctionStructureTokens(true);
        lexWhitespaceToken();
        lexOperatorToken(';');
        lexWhitespaceToken();
    }while(lexNewLineChars());
}

bool Lexer::lexInterfaceStructureTokens() {
    if (lexKeywordToken("interface")) {
        unsigned start = tokens.size() - 1;
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
        lexInterfaceBlockTokens();
        lexWhitespaceToken();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface");
            return true;
        }
        compound_from<InterfaceCST>(start);
        return true;
    }
    return false;
}