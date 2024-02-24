// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include <utility>
#include "lexer/minLsp/SemanticTokens.h"
#include "lexer/model/TokenPosition.h"

class LexToken {
public:

    TokenPosition position;

    LexToken(const TokenPosition& position) : position(position) {

    }

    inline unsigned int start() {
        return position.position;
    }

    inline unsigned int end() {
        return position.position + length();
    }

    inline unsigned int lineNumber() {
        return position.lineNumber;
    }

    inline unsigned int lineCharNumber() {
        return position.lineCharNumber;
    }

    virtual unsigned int length() const = 0;

    virtual LspSemanticTokenType lspType() const = 0;

    virtual std::string type_string() const = 0;

    virtual std::string content() const = 0;

};
