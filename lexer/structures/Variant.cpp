// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexVariantMemberTokens() {
    if(lexIdentifierToken()) {
        unsigned start = tokens.size() - 1;
        if(lexOperatorToken('(')) {
            lexParameterList(false, true, false, false);
            if(!lexOperatorToken(')')) {
                error("expected a ')' after variant member");
                return true;
            }
            compound_from(start, LexTokenType::CompVariantMember);
        } else {
            error("expected a '(' after identifier in variant member");
        }
        return true;
    } else {
        return false;
    }
}

void Lexer::lexVariantBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(
            lexFunctionStructureTokens() ||
            lexSingleLineCommentTokens() ||
            lexMultiLineCommentTokens() ||
            lexAnnotationMacro() ||
            lexVariantMemberTokens()
        )) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Lexer::lexVariantStructureTokens() {
    if(lexKeywordToken("variant")) {
        auto start_token = tokens.size() - 1;
        lexWhitespaceToken();
        if (!lexIdentifierToken()) {
            error("expected a identifier as struct name");
            return true;
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
        lexVariantBlockTokens();
        isCBICollecting = prev;
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for struct block");
            return true;
        }
        compound_collectable(start_token, LexTokenType::CompVariant);
        return true;
    } else {
        return false;
    }
}