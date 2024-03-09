// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/InterfaceToken.h"

void Lexer::lexInterfaceBlockTokens() {
    do {
        lexWhitespace();
        lexFunctionSignatureTokens() || lexVarInitializationTokens(true);
        lexWhitespace();
        lexOperatorToken(';');
        lexWhitespace();
    } while (lexNewLineChars());
}

bool Lexer::lexInterfaceStructureTokens() {
    if (lexKeywordToken("interface")) {
        lexWhitespaceToken();
        auto name = lexAlpha();
        if (name.empty()) {
            error("expected interface name after the interface keyword");
            return true;
        } else {
            tokens.emplace_back(std::make_unique<InterfaceToken>(backPosition(name.length()), std::move(name)));
        }
        lexWhitespace();
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an interface");
            return true;
        }
        lexWhitespace();
        lexNewLineChars();
        lexInterfaceBlockTokens();
        lexWhitespace();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an interface");
            return true;
        }
    } else {
        return false;
    }
}