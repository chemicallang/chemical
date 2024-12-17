// Copyright (c) Qinetik 2024.

#pragma once
#include "std/chem_string_view.h"

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
    enum TokenType type;

    /**
     * the value is the string in the token
     */
    chem::string_view value;

    /**
     * the location is where the token is
     */
    Position position;

    /**
     * check if given token type is a keyword
     */
    static inline bool isKeyword(enum TokenType type) {
        return type > TokenType::IndexKwStart && type < TokenType::IndexKwEnd;
    }

    /**
     * check if given token type is a keyword or identifier
     */
    static inline bool isKeywordOrId(enum TokenType type) {
        return type == TokenType::Identifier || isKeyword(type);
    }

};