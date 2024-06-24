// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/structures/UnionCST.h"

bool Lexer::lexUnionMemberTokens() {
    return lexVarInitializationTokens(true, true) ||
            lexFunctionStructureTokens() ||
            lexSingleLineCommentTokens() ||
            lexMultiLineCommentTokens() ||
            lexAnnotationMacro();
}

void Lexer::lexUnionBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!lexUnionMemberTokens()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Lexer::lexUnionStructureTokens() {
    if(lexKeywordToken("union")) {
        auto start_token = tokens.size() - 1;
        lexWhitespaceToken();
        if(!lexIdentifierToken()) {
            error("expected a identifier as union name");
            return true;
        }
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            lexWhitespaceToken();
            if(!lexVariableToken()) {
                error("expected a interface name after ':' when declaring a union");
                return true;
            }
        }
        lexWhitespaceToken();
        if(!lexOperatorToken('{')) {
            error("expected a '{' for union block");
            return true;
        }
        auto prev = isCBICollecting;
        isCBICollecting = false;
        lexUnionBlockTokens();
        isCBICollecting = prev;
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for union block");
            return true;
        }
        lexWhitespaceToken();
        compound_collectable<UnionDefCST>(start_token);
        return true;
    } else {
        return false;
    }
}