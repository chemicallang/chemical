// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <span>
#include "lexer/Token.h"
#include "core/diag/Position.h"

Token* get_token_at_position(const std::span<Token>& tokens, const Position& position);