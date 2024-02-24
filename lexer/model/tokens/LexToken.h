// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include <utility>
#include "lexer/minLsp/SemanticTokens.h"

class LexToken {
public:
    unsigned int start;
    unsigned int length;
    unsigned int lineNumber;

    LexToken(unsigned int start, unsigned int length, unsigned int lineNumber) : start(start), length(length), lineNumber(lineNumber) {

    }

//    /**
//     * this returns the representation of source, for example variable declaration token returns "var"
//     * @return source representation of token
//     */
//    virtual std::string representation() = 0;

    inline unsigned int end() {
        return start + length;
    }

    virtual LspSemanticTokenType lspType() const = 0;

    virtual std::string type_string() const = 0;

    virtual std::string content() const = 0;

};
