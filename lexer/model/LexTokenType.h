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
    Keyword,
    MultilineComment,
    Number,
    StringOperator,
    String,
    Type,
    Bool,
    Variable,
    Macro,
    RawToken,
    UserToken,
    Compound
};