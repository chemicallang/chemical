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

bool Parser::check_type(LexTokenType type, bool errorOut) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == type) {
            return true;
        } else if(errorOut){
            error("expected a " + toTypeString(type) + " token, got " + toTypeString(tokens[position]->type()));
        }
    } else if(errorOut){
        error("expected a " + toTypeString(type) + " token but there are no tokens left");
    }
    return false;
}

void Parser::print_got() {
    std::string err;
    err.append("got ");
    err.append(toTypeString(tokens[position]->type()));
    std::cout << err;
    error(err);
}

bool Parser::consume_op(char token) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == LexTokenType::CharOperator) {
            if (as<CharOperatorToken>()->op == token) {
                increment();
                return true;
            }
        }
    }
    return false;
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