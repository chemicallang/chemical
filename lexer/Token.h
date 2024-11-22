// Copyright (c) Qinetik 2024.

#pragma once

#include <string_view>
#include "TokenType.h"
#include "integration/common/Position.h"

/**
 * the token lexed by our lexer
 */
struct Token {

    /**
     * the type of the token
     */
    TokenType type;

    /**
     * the value is the string in the token
     */
    std::string_view value;

    /**
     * the location is where the token is
     */
    Position position;

};