// Copyright (c) Chemical Language Foundation 2025.

#include "AnalyzerUtils.h"

bool is_position_inside_token(const Position& position, const Token& token) {
    auto& tPos = token.position;
    return position.line == tPos.line && position.character >= tPos.character && position.character <= (tPos.character + token.value.size());
}

Token* get_token_at_position(const std::span<Token>& tokens, const Position& position) {
    for(auto& t : tokens) {
        if(is_position_inside_token(position, t)) {
            return &t;
        }
    }
    return nullptr;
}