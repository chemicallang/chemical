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
    Class,
    Comment,
    EnumMember,
    Enum,
    Function,
    Interface,
    Keyword,
    Method,
    Modifier,
    MultilineComment,
    Number,
    Parameter,
    Property,
    StringOperator,
    String,
    Struct,
    Type,
    Bool,
    Variable,
    Macro,
    RawToken,
    UserToken,
    Compound
};