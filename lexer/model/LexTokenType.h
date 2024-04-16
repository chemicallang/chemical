// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <cstdint>

enum class LexTokenType : uint8_t {

    CharOperator,
    Operation,
    Char,
    Comment,

    // absolute string starts here
    Keyword,
    Number,
    Type,
    StringOperator,
    Variable,
    RawToken,
    // absolute string ends here

    MultilineComment,
    String,
    Bool,
    Macro,
    UserToken,
    Compound,

    IndexLastToken = Compound,
    IndexAbsStrStart = Keyword,
    IndexAbsStrEnd = RawToken,

};