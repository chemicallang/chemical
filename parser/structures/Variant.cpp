// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexVariantMemberTokens() {
    if(lexIdentifierToken()) {
        unsigned start = tokens_size() - 1;
        if(lexOperatorToken(TokenType::LParen)) {
            lexParameterList(false, true, false, false);
            if(!lexOperatorToken(TokenType::RParen)) {
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

void Parser::lexVariantBlockTokens() {
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
        lexOperatorToken(TokenType::SemiColonSym);
    } while(token->type != TokenType::RBrace);
    lexWhitespaceToken();
}

bool Parser::lexVariantStructureTokens(unsigned start_token) {
    if(lexWSKeywordToken(TokenType::VariantKw)) {
        if (!lexIdentifierToken()) {
            error("expected a identifier as struct name");
            return true;
        }
        lexWhitespaceToken();
        lexGenericParametersList();
//        lexWhitespaceToken();
//        if(lexOperatorToken(':')) {
//            do {
//                lexWhitespaceToken();
//                lexAccessSpecifier(false, true);
//                if(!lexRefOrGenericType()) {
//                    return true;
//                }
//                lexWhitespaceToken();
//            } while(lexOperatorToken(','));
//        }
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return true;
        }
        lexVariantBlockTokens();
        if(!lexOperatorToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return true;
        }
        compound_from(start_token, LexTokenType::CompVariant);
        return true;
    } else {
        return false;
    }
}