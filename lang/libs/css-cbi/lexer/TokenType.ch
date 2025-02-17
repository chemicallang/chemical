enum TokenType {
    // end of file
    EndOfFile,
    // unexpected
    Unexpected,
    // identifier
    Identifier,
    // floating or non floating number
    Number,
    // comment begins double forward slash
    Comment,
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
    // #e3e3e3
    HexColor,
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