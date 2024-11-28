// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#include "parser/Parser.h"

void Parser::lexImplBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(lexFunctionStructureTokens() || lexSingleLineCommentTokens() || lexMultiLineCommentTokens() || lexAnnotationMacro())) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(TokenType::SemiColonSym);
    } while(token->type != TokenType::RBrace);
}

bool Parser::lexImplTokens() {
    if (lexWSKeywordToken(TokenType::ImplKw)) {
        auto start = tokens_size() - 1;
        lexGenericParametersList();
        lexWhitespaceToken();
        if(!lexRefOrGenericType()) return true;
        lexWhitespaceToken();
        if(lexWSKeywordToken(TokenType::ForKw)) {
            if(!lexRefOrGenericType()) return true;
            lexWhitespaceToken();
        }
        if (!lexOperatorToken(TokenType::LBrace)) {
            mal_node(start, "expected a '{' when starting an implementation");
            return true;
        }
        lexImplBlockTokens();
        if (!lexOperatorToken(TokenType::RBrace)) {
            mal_node(start,"expected a '}' when ending an implementation");
            return true;
        }
        compound_from(start, LexTokenType::CompImpl);
        return true;
    }
    return false;
}