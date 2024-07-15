// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/Lexer.h"
#include "lexer/model/tokens/BoolToken.h"
#include "lexer/model/tokens/NullToken.h"

const std::unordered_map<std::string, ValueCreatorFn> ValueCreators = {
        {"null", [](Lexer *lexer) -> void {
            lexer->tokens.emplace_back(std::make_unique<NullToken>(LexTokenType::Null, lexer->backPosition(4), "null"));
        }},
        {"true", [](Lexer *lexer) -> void {
            lexer->tokens.emplace_back(std::make_unique<BoolToken>(LexTokenType::Bool, lexer->backPosition(4), "true"));
        }},
        {"false", [](Lexer *lexer) -> void {
            lexer->tokens.emplace_back(std::make_unique<BoolToken>(LexTokenType::Bool, lexer->backPosition(5), "false"));
        }}
};