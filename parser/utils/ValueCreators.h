// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/Parser.h"
#include <unordered_map>

const std::unordered_map<std::string, ValueCreatorFn> ValueCreators = {
        {"null", [](Parser *lexer) -> void {
            lexer->emplace(LexTokenType::Null, lexer->backPosition(4), "null");
        }},
        {"true", [](Parser *lexer) -> void {
            lexer->emplace(LexTokenType::Bool, lexer->backPosition(4), "true");
        }},
        {"false", [](Parser *lexer) -> void {
            lexer->emplace(LexTokenType::Bool, lexer->backPosition(5), "false");
        }}
};