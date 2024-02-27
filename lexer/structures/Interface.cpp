// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

void Lexer::lexInterfaceBlockTokens() {
    do {
        lexWhitespace();
        lexFunctionSignatureTokens();
        lexWhitespace();
        lexOperatorToken(';');
        lexWhitespace();
    }while(lexNewLineChars());
}

bool Lexer::lexInterfaceStructureTokens() {
    if(lexKeywordToken("interface")) {
        lexWhitespaceToken();
        auto name = lexAlpha();
        if(name.empty()) {
            error("expected interface name after the interface keyword");
            return true;
        }
        lexWhitespace();
        if(!lexOperatorToken('{')) {
            error("expected a '{' when starting an interface");
            return true;
        }
        lexWhitespace();
        lexNewLineChars();
        lexInterfaceBlockTokens();
        lexWhitespace();
        if(!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface");
            return true;
        }
    } else {
        return false;
    }
}