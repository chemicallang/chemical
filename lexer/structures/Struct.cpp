// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexStructMemberTokens() {
    return lexVarInitializationTokens(true, true) ||
            lexFunctionStructureTokens() ||
            lexSingleLineCommentTokens() ||
            lexMultiLineCommentTokens() ||
            lexUnionStructureTokens(true, true) ||
            lexStructStructureTokens(true, true) ||
            lexAnnotationMacro();
}

void Lexer::lexStructBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!lexStructMemberTokens()) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Lexer::lexStructStructureTokens(bool unnamed, bool direct_init) {
    if(lexWSKeywordToken("struct")) {
        auto start_token = tokens.size() - 1;
        bool has_identifier = false;
        if(!unnamed) {
            has_identifier = lexIdentifierToken();
            if (!has_identifier) {
                error("expected a identifier as struct name");
                return true;
            }
        }
        lexWhitespaceToken();
        lexGenericParametersList();
        lexWhitespaceToken();
        if(lexOperatorToken(':')) {
            do {
                lexWhitespaceToken();
                lexAccessSpecifier(false, true);
                lexWhitespaceToken();
                if(!lexRefOrGenericType()) {
                    return true;
                }
                lexWhitespaceToken();
            } while(lexOperatorToken(','));
        }
        lexWhitespaceToken();
        if(!lexOperatorToken('{')) {
            error("expected a '{' for struct block");
            return true;
        }
        auto prev = isCBICollecting;
        isCBICollecting = false;
        lexStructBlockTokens();
        isCBICollecting = prev;
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for struct block");
            return true;
        }
        if(lexWhitespaceToken() && !has_identifier && direct_init && !lexIdentifierToken()) {
            error("expected an identifier after the '}' for anonymous struct definition");
            return true;
        }
        compound_collectable(start_token, LexTokenType::CompStructDef);
        return true;
    } else {
        return false;
    }
}