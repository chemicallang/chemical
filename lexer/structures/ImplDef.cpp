// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/InterfaceToken.h"
#include "lexer/model/tokens/StructToken.h"

void Lexer::lexImplBlockTokens() {
    do {
        lexWhitespaceToken();
        lexFunctionStructureTokens() || lexVarInitializationTokens();
        lexWhitespaceToken();
        lexOperatorToken(';');
        lexWhitespaceToken();
    } while (lexNewLineChars());
}

bool Lexer::lexImplTokens() {
    if (lexKeywordToken("impl")) {
        lexWhitespaceToken();
        auto name = lexAlpha();
        if (name.empty()) {
            error("expected interface name after the interface keyword in implementation");
            return true;
        } else {
            tokens.emplace_back(std::make_unique<InterfaceToken>(backPosition(name.length()), std::move(name)));
        }
        lexWhitespaceToken();
        if(!lexKeywordToken("for")) {
            error("expected 'for' in impl block for interface " + name);
            return true;
        }
        lexWhitespaceToken();
        auto struct_name = lexAlpha();
        if(struct_name.empty()) {
            error("expected a struct name after the 'for' keyword in implementation");
            return true;
        } else {
            tokens.emplace_back(std::make_unique<StructToken>(backPosition(struct_name.length()), std::move(struct_name)));
        }
        lexWhitespaceToken();
        if (!lexOperatorToken('{')) {
            error("expected a '{' when starting an implementation");
            return true;
        }
        lexWhitespaceToken();
        lexNewLineChars();
        lexImplBlockTokens();
        lexWhitespaceAndNewLines();
        if (!lexOperatorToken('}')) {
            error("expected a '}' when ending an implementation");
            return true;
        }
        return true;
    }
    return false;
}