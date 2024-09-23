// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/Lexer.h"
#include <unordered_map>

const std::unordered_map<std::string, ValueCreatorFn> ValueCreators = {
        {"null", [](Lexer *lexer) -> void {
            lexer->emplace(LexTokenType::Null, lexer->backPosition(4), "null");
        }},
        {"true", [](Lexer *lexer) -> void {
            lexer->emplace(LexTokenType::Bool, lexer->backPosition(4), "true");
        }},
        {"false", [](Lexer *lexer) -> void {
            lexer->emplace(LexTokenType::Bool, lexer->backPosition(5), "false");
        }}
};