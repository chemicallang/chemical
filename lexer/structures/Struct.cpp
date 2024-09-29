// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexStructMemberTokens() {
    if(lexVarInitializationTokens(true, true)) {
        auto& last = unit.tokens.back();
#ifdef DEBUG
        if(last->tok_type != LexTokenType::CompVarInit) {
            throw std::runtime_error("unknown compound token, expected var init");
        }
#endif
        last->tok_type = LexTokenType::CompStructMember;
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
                mal_node(start, "expected a identifier as struct name");
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
            mal_node(start, "expected a '{' for struct block");
            return true;
        }
        lexStructBlockTokens();
        if(!lexOperatorToken('}')) {
            mal_node(start, "expected a closing bracket '}' for struct block");
            return true;
        }
        if(lexWhitespaceToken() && !has_identifier && direct_init && !lexIdentifierToken()) {
            mal_node(start, "expected an identifier after the '}' for anonymous struct definition");
            return true;
        }
        compound_from(start, LexTokenType::CompStructDef);
        return true;
    } else {
        return false;
    }
}