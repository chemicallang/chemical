// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <vector>
#include <memory>
#include "lexer/model/tokens/LexToken.h"
#include "lexer/Lexer.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/IdentifierToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "utils/FileUtils.h"

bool Lexer::lexVarInitializationTokens(bool allowDeclarations) {

    if (!lexKeywordToken("var")) {
        return false;
    }

    // whitespace
    lexWhitespaceToken();

    // identifier
    if (!lexIdentifierTokenBool()) {
        error("expected an identifier for variable initialization");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // :
    if (lexOperatorToken(':')) {

        // whitespace
        lexWhitespaceToken();

        // type
        lexTypeTokens();

        // whitespace
        lexWhitespaceToken();

    }

    // equal sign
    if (!lexOperatorToken('=')) {
        if(allowDeclarations) {
            lexOperatorToken(';');
        } else {
            error("expected an = sign for the initialization of the variable");
        }
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // value
    if (!lexValueToken()) {
        error("expected a value for variable initialization");
        return true;
    }

    return true;

}