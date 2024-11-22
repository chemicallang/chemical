// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Lexer.h"

bool Lexer::lexStructMemberTokens() {
    if(lexVarInitializationTokens(true, true)) {
        auto& last = unit.tokens.back();
        if(last->tok_type == LexTokenType::CompVarInit) {
            last->tok_type = LexTokenType::CompStructMember;
        }
        return true;
    } else {
        return false;
    }
}

void Lexer::lexStructBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(
            lexStructMemberTokens() ||
            lexFunctionStructureTokens(true) ||
            lexSingleLineCommentTokens() ||
            lexMultiLineCommentTokens() ||
            lexUnionStructureTokens(true, true) ||
            lexStructStructureTokens(true, true) ||
            lexAnnotationMacro()
        )) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Lexer::lexStructStructureTokens(unsigned start, bool unnamed, bool direct_init) {
    if(lexWSKeywordToken("struct")) {
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
        lexStructBlockTokens();
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for struct block");
            return true;
        }
        if(lexWhitespaceToken() && !has_identifier && direct_init && !lexIdentifierToken()) {
            error("expected an identifier after the '}' for anonymous struct definition");
            return true;
        }
        compound_from(start, LexTokenType::CompStructDef);
        return true;
    } else {
        return false;
    }
}