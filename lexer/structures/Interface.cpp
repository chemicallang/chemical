// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/InterfaceToken.h"

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
        auto name = lexAlpha();
        if (name.empty()) {
            error("expected interface name after the interface keyword");
            return true;
        } else {
            tokens.emplace_back(std::make_unique<InterfaceToken>(backPosition(name.length()), std::move(name)));
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
    } else {
        return false;
    }
}