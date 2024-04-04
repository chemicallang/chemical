// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/RawToken.h"
#include "lexer/model/tokens/StringOperatorToken.h"
#include "lexer/model/tokens/CharToken.h"
#include "lexer/model/tokens/StringToken.h"
#include "lexer/model/tokens/OperationToken.h"
#include "lexer/utils/TypeUtils.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/KeywordToken.h"

bool Parser::check_type(LexTokenType type, bool errorOut) {
    if (position < tokens.size()) {
        if (tokens[position]->type() == type) {
            return true;
        } else if (errorOut) {
            error("expected a " + toTypeString(type) + " token, got " + toTypeString(tokens[position]->type()));
        }
    } else if (errorOut) {
        error("expected a " + toTypeString(type) + " token but there are no tokens left");
    }
    return false;
}

void Parser::print_got() {
    std::string err;
    err.append("got ");
    err.append(toTypeString(tokens[position]->type()));
    std::cout << err << " at " + std::to_string(tokens[position]->lineNumber()) + ":" +
                        std::to_string(tokens[position]->lineCharNumber());
    error(err);
}

std::optional<Operation> Parser::consume_op_token() {
    if (position < tokens.size() && tokens[position]->type() == LexTokenType::Operation) {
        return consume<OperationToken>()->op;
    }
    return std::nullopt;
}

std::optional<std::string> Parser::consume_str_token() {
    if (position < tokens.size() && tokens[position]->type() == LexTokenType::String) {
        return consume<StringToken>()->value;
    }
    return std::nullopt;
}

std::optional<char> Parser::consume_char_token() {
    if (position < tokens.size() && tokens[position]->type() == LexTokenType::Char) {
        return consume<CharToken>()->value;
    }
    return std::nullopt;
}

std::optional<char> Parser::get_op_token() {
    if (position < tokens.size() && tokens[position]->type() == LexTokenType::CharOperator) {
        return as<CharOperatorToken>()->op;
    }
    return std::nullopt;
}

std::optional<std::string> Parser::get_str_op_token() {
    if (position < tokens.size() && tokens[position]->type() == LexTokenType::StringOperator) {
        return as<StringOperatorToken>()->op;
    }
    return std::nullopt;
}

bool Parser::consume_op(char token) {
    auto v = get_op_token();
    if (v.has_value() && v.value() == token) {
        position++;
        return true;
    } else {
        return false;
    }
}

bool Parser::consume_op(const std::string& token) {
    auto v = get_str_op_token();
    if (v.has_value() && v.value() == token) {
        position++;
        return true;
    } else {
        return false;
    }
}

bool Parser::consume(const std::string &keyword) {
    if (tokens.size() != position && tokens[position]->type() == LexTokenType::Keyword &&
        as<KeywordToken>()->keyword == keyword) {
        position++;
        return true;
    } else {
        return false;
    }
}

KeywordToken* Parser::consume(const std::string &keyword, bool errorOut) {
    if (tokens.size() != position) {
        if (tokens[position]->type() == LexTokenType::Keyword) {
            if (as<KeywordToken>()->keyword == keyword) {
                return consume<KeywordToken>();
            } else if (errorOut) {
                error("expected a '" + keyword + "' keyword, got " + as<KeywordToken>()->keyword);
            }
        } else if (errorOut) {
            error("expected a " + toTypeString(LexTokenType::Keyword) + " token, got " +
                  toTypeString(tokens[position]->type()));
        }
    } else if (errorOut) {
        error("expected a " + toTypeString(LexTokenType::Keyword) + " token but there are no tokens left");
    }
    return nullptr;
}