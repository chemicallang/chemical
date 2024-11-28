// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/Parser.h"
#include <unordered_map>

const std::unordered_map<std::string_view, ValueCreatorFn> ValueCreators = {
        {"null", [](Parser *lexer, Position& pos) -> void {
            lexer->emplace(LexTokenType::Null, pos, "null");
        }},
        {"true", [](Parser *lexer, Position& pos) -> void {
            lexer->emplace(LexTokenType::Bool, pos, "true");
        }},
        {"false", [](Parser *lexer, Position& pos) -> void {
            lexer->emplace(LexTokenType::Bool, pos, "false");
        }}
};