// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "lexer/model/tokens/AbstractStringToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"

inline std::string str_token(CSTToken *token) {
    return static_cast<AbstractStringToken *>(token)->value;
}

inline std::string escaped_str_token(CSTToken *token) {
    auto str = str_token(token);
    return str.substr(1, str.size() - 2);
}

inline char char_op(CSTToken *token) {
    return static_cast<CharOperatorToken *>(token)->value[0];
}

inline bool is_keyword(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Keyword && str_token(token) == x;
}

inline bool is_variable(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Variable && str_token(token) == x;
}

inline bool is_char_op(CSTToken *token, char x) {
    return token->type() == LexTokenType::CharOperator && char_op(token) == x;
}

inline bool is_str_op(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::StringOperator && str_token(token) == x;
}