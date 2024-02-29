// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/WhitespaceToken.h"

void Parser::eraseAllWhitespaceTokens() {
    auto i = 0;
    while (i < tokens.size()) {
        if (tokens[position]->type() == LexTokenType::Whitespace) {
            tokens.erase(tokens.begin() + i);
        }
        i++;
    }
}

lex_ptr<CharOperatorToken> Parser::consumeOperator(char token, bool errorOut) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == LexTokenType::CharOperator) {
            if (as<CharOperatorToken>()->op == token) {
                return consume<CharOperatorToken>();
            } else {
                std::string err;
                err.append("expected a '");
                err.append(1, token);
                err.append("' keyword, got '");
                err.append(1, as<CharOperatorToken>()->op);
                err.append(1, '\'');
                error(err);
            }
        } else if(errorOut) {
            error("expected a " + toTypeString(LexTokenType::CharOperator) + " token, got " +
                  toTypeString(tokens[position]->type()));
        }
    } else if(errorOut) {
        error("expected a " + toTypeString(LexTokenType::CharOperator) + " token but there are no tokens left");
    }
    return std::nullopt;
}

lex_ptr<KeywordToken> Parser::consume(const std::string &keyword, bool errorOut) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == LexTokenType::Keyword) {
            if (as<KeywordToken>()->keyword == keyword) {
                return consume<KeywordToken>();
            } else {
                error("expected a '" + keyword + "' keyword, got" + as<KeywordToken>()->keyword);
            }
        } else if(errorOut) {
            error("expected a " + toTypeString(LexTokenType::Keyword) + " token, got " +
                  toTypeString(tokens[position]->type()));
        }
    } else if(errorOut){
        error("expected a " + toTypeString(LexTokenType::Keyword) + " token but there are no tokens left");
    }
    return std::nullopt;
}