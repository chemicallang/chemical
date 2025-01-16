enum TokenType {
    // end of file
    EndOfFile,
    // unexpected
    Unexpected,
    // identifier
    Identifier,
    // floating or non floating number
    Number,
    // enclosed in double quotes
    DoubleQuotedValue,
    // enclosed in single quotes
    SingleQuotedValue,
    // :
    Colon,
    // ;
    Semicolon,
    // @
    At,
    // !important
    Important,
    // {
    LBrace,
    // }
    RBrace,
    // (
    LParen,
    // )
    RParen,
    // %
    Percentage,
    // ,
    Comma,
    // +
    Plus,
    // -
    Minus,
    // *
    Multiply,
    // /
    Divide,
    // =
    Equal,
    // >
    GreaterThan,
    // >=
    GreaterThanOrEqual,
    // <
    LessThan,
    // <=
    LessThanOrEqual,
    // ~=
    ContainsWord,
    // *=
    ContainsSubstr,
    // ^=
    StartsWith,
    // $=
    EndsWith,
    // |=
    DashSeparatedMatch,
    // ~
    GeneralSibling,
}