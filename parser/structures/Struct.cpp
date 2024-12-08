// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexStructMemberTokens() {
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

void Parser::lexStructBlockTokens() {
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
        lexOperatorToken(TokenType::SemiColonSym);
    } while(token->type != TokenType::RBrace);
    lexWhitespaceToken();
}

bool Parser::lexStructStructureTokens(unsigned start, bool unnamed, bool direct_init) {
    if(lexWSKeywordToken(TokenType::StructKw)) {
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
        if(lexOperatorToken(TokenType::ColonSym)) {
            do {
                lexWhitespaceToken();
                lexAccessSpecifier(false, true);
                if(!lexRefOrGenericType()) {
                    return true;
                }
                lexWhitespaceToken();
            } while(lexOperatorToken(TokenType::CommaSym));
        }
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return true;
        }
        lexStructBlockTokens();
        if(!lexOperatorToken(TokenType::RBrace)) {
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