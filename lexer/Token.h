// Copyright (c) Chemical Language Foundation 2025.

#pragma once
#include "std/chem_string_view.h"

#include <string_view>
#include "TokenType.h"
#include "core/diag/Position.h"

class ASTAny;

/**
 * the token lexed by our lexer
 */
struct Token {

    /**
     * the type of the token
     */
    enum TokenType type;

    /**
     * the string value of the token
     */
    chem::string_view value;

    /**
     * the position of the token in source code
     */
    Position position;

#ifdef LSP_BUILD
    /**
     * the linked node / value / type that was created from this token
     * this is only present in lsp build
     */
    ASTAny* linked = nullptr;
#endif

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