// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexOperatorToken(char op) {
    if(provider.increment(op)) {
        emplace(LexTokenType::CharOperator, backPosition(1), std::string(1, op));
        return true;
    } else {
        return false;
    }
}

bool Parser::lexOperatorToken(const std::string_view& op) {
    if(provider.increment(op)) {
        emplace(LexTokenType::StringOperator, backPosition(op.length()), std::string(op));
        return true;
    } else {
        return false;
    }
}

void Parser::storeOperationToken(char token, Operation op) {
    std::string value;
    value.append(std::to_string((int) op));
    value.append(1, token);
    emplace(LexTokenType::Operation, backPosition(1), std::move(value));
}

bool Parser::lexOperationToken(char token, Operation op) {
    if(provider.increment(token)) {
        storeOperationToken(token, op);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexOperatorToken(const std::string_view& token, Operation op) {
    if(provider.increment(token)) {
        std::string value;
        value.append(std::to_string((int) op));
        value.append(token);
        emplace(LexTokenType::Operation, backPosition(token.length()), value);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexWSKeywordToken(const std::string_view& keyword) {
    if(provider.increment(keyword, true) && provider.peek(keyword.size()) == ' ') {
        provider.increment_amount(keyword.size());
        emplace(LexTokenType::Keyword, backPosition(keyword.length()), keyword);
        lexWhitespaceToken();
        return true;
    } else {
        return false;
    }
}

bool Parser::lexWSKeywordToken(const std::string_view& keyword, char may_end_at) {
    if(provider.increment(keyword, true)) {
        const auto peek = provider.peek(keyword.size());
        if(peek == ' ' || peek == '\t') {
            provider.increment_amount(keyword.size());
            emplace(LexTokenType::Keyword, backPosition(keyword.length()), keyword);
            lexWhitespaceToken();
            return true;
        } else if(peek == may_end_at) {
            provider.increment_amount(keyword.size());
            emplace(LexTokenType::Keyword, backPosition(keyword.length()), keyword);
            return true;
        }
    }
    return false;
}

bool Parser::lexKeywordToken(const std::string_view& keyword) {
    if(provider.increment(keyword)) {
        emplace(LexTokenType::Keyword, backPosition(keyword.length()), keyword);
        return true;
    } else {
        return false;
    }
}

static bool read_gen_type_token(Parser& lexer);

static bool read_arr_type_token(Parser& lexer);

static bool read_pointer_type(Parser& lexer);

static bool read_type_involving_token(Parser& lexer) {
    auto& provider = lexer.provider;
    if(!provider.readIdentifier().empty()) {
        read_gen_type_token(lexer) || read_arr_type_token(lexer) || read_pointer_type(lexer);
        return true;
    } else {
        return false;
    }
}

static bool read_pointer_type(Parser& lexer) {
    auto& provider = lexer.provider;
    if(provider.increment('*')) {
        while(provider.increment('*')) {
            // do nothing
        }
        return true;
    } else {
        return false;
    }
}

static bool read_arr_type_token(Parser& lexer) {
    auto& provider = lexer.provider;
    if(provider.increment('[')) {
        provider.readUnsignedInt();
        if(!provider.increment(']')) {
            lexer.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

static bool read_gen_type_token(Parser& lexer) {
    auto& provider = lexer.provider;
    if(provider.increment('<')) {
        read_type_involving_token(lexer);
        if(!provider.increment('>')) {
            lexer.error("unknown token in look ahead operation for generics, expected '>'");
        }
        return true;
    } else {
        return false;
    }
}

bool Parser::isGenericEndAhead() {
    const auto position = provider.getStreamPosition();
    provider.increment('<');
    auto& lexer = *this;
    do {
        readWhitespace();
        if (!read_type_involving_token(lexer)) {
            provider.restore(position);
            return false;
        }
        readWhitespace();
    } while (provider.increment(','));
    const bool is_generic = provider.increment('>');
    provider.restore(position);
    return is_generic;
}

bool Parser::lexAccessSpecifier(bool internal, bool protect) {
    return lexWSKeywordToken("public") || lexWSKeywordToken("private") || (internal && lexWSKeywordToken("internal")) || (protect && lexWSKeywordToken("protected"));
}