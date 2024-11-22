// Copyright (c) Qinetik 2024.

#pragma once

enum class TokenType {

    // if, while
    Keyword,

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

    // +, -, &, &&, || arithmetic or logical operators
    Operator,

    // a number, doesn't include the negative sign
    Number,

    // when reached the end of file
    EndOfFile,

    // usually means an error should occur
    Unexpected



};