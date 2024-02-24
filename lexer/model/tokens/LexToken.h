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
    unsigned int lineNumber;

    LexToken(unsigned int start, unsigned int lineNumber) : start(start), lineNumber(lineNumber) {

    }

    virtual unsigned int length() const = 0;

    inline unsigned int end() {
        return start + length();
    }

    virtual LspSemanticTokenType lspType() const = 0;

    virtual std::string type_string() const = 0;

    virtual std::string content() const = 0;

};
