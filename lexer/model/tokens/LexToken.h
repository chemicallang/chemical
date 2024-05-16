// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include <optional>
#include "lexer/model/LexTokenType.h"
#include "common/Diagnostic.h"
#include "cst/base/CSTToken.h"

class LexToken : public CSTToken {
public:

    Position position;
    std::string value;

    LexToken(const Position &position, std::string value) : position(position), value(std::move(value)) {
        this->value.shrink_to_fit();
    }

    inline unsigned int lineNumber() {
        return position.line;
    }

    inline unsigned int lineCharNumber() {
        return position.character;
    }

    /**
     * string length of the token
     */
    inline unsigned int length() const {
        return value.size();
    }

    /**
     * lex token virtual destructor
     */
    virtual ~LexToken() = default;

};
