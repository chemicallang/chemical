// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/InterfaceCST.h"

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
    if (lexKeywordToken("interface")) {
        unsigned start = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexVariableToken()) {
            error("expected interface name after the interface keyword");
            return true;
        }
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
        compound_from<InterfaceCST>(start);
        return true;
    }
    return false;
}