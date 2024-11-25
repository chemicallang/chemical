// Copyright (c) Qinetik 2024.

#pragma once

enum class TokenType {

    // var, func, if, while... are keywords
    Keyword,

    // anything not part of the language
    Identifier,

    // whitespace tokens
    Whitespace,

    // new line character
    NewLine,

    LParen, // (
    RParen, // )

    LBrace, // {
    RBrace, // }

    LBracket, // ]
    RBracket, // ]

    // when the value is inside double quotes
    String,
    // when the value is inside single quotes
    Char,

    // escape sequence inside quotes
    EscapeSeq,

    // when this text is part of a single line comment
    SingleLineComment,

    // when this text is part of a multi line comment
    MultiLineComment,

    // +, -, &, &&, || arithmetic or logical operators
    Operator,

    // a number, doesn't include the negative sign
    Number,

    // when reached the end of file
    EndOfFile,

    // usually means an error should occur
    Unexpected



};