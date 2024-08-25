// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexOperatorToken(char op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::CharOperator, backPosition(1), std::string(1, op)));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperatorToken(const std::string& op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::StringOperator, backPosition(op.length()), op));
        return true;
    } else {
        return false;
    }
}

void Lexer::storeOperationToken(char token, Operation op) {
    std::string value;
    value.append(std::to_string((int) op));
    value.append(1, token);
    tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Operation, backPosition(1), std::move(value)));
}

bool Lexer::lexOperationToken(char token, Operation op) {
    if(provider.increment(token)) {
        storeOperationToken(token, op);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperatorToken(const std::string &token, Operation op) {
    if(provider.increment(token)) {
        std::string value;
        value.append(std::to_string((int) op));
        value.append(token);
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Operation, backPosition(token.length()), value));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexWSKeywordToken(const std::string &keyword) {
    if(provider.increment(keyword, true) && provider.peek(keyword.size()) == ' ') {
        provider.increment_amount(keyword.size());
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Keyword, backPosition(keyword.length()), keyword));
        lexWhitespaceToken();
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexWSKeywordToken(const std::string &keyword, char may_end_at) {
    if(provider.increment(keyword, true)) {
        const auto peek = provider.peek(keyword.size());
        if(peek == ' ' || peek == '\t') {
            provider.increment_amount(keyword.size());
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Keyword, backPosition(keyword.length()), keyword));
            lexWhitespaceToken();
            return true;
        } else if(peek == may_end_at) {
            provider.increment_amount(keyword.size());
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Keyword, backPosition(keyword.length()), keyword));
            return true;
        }
    }
    return false;
}

bool Lexer::lexKeywordToken(const std::string& keyword) {
    if(provider.increment(keyword)) {
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Keyword, backPosition(keyword.length()), keyword));
        return true;
    } else {
        return false;
    }
}

static bool read_gen_type_token(Lexer& lexer);

static bool read_arr_type_token(Lexer& lexer);

static bool read_pointer_type(Lexer& lexer);

static bool read_type_involving_token(Lexer& lexer) {
    auto& provider = lexer.provider;
    if(!provider.readIdentifier().empty()) {
        read_gen_type_token(lexer) || read_arr_type_token(lexer) || read_pointer_type(lexer);
        return true;
    } else {
        return false;
    }
}

static bool read_pointer_type(Lexer& lexer) {
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

static bool read_arr_type_token(Lexer& lexer) {
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

static bool read_gen_type_token(Lexer& lexer) {
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

bool Lexer::isGenericEndAhead() {
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

bool Lexer::lexAccessSpecifier(bool internal, bool protect) {
    return lexWSKeywordToken("public") || lexWSKeywordToken("private") || (internal && lexWSKeywordToken("internal")) || (protect && lexWSKeywordToken("protected"));
}