enum TokenType {
    // end of file
    EndOfFile,
    // unexpected
    Unexpected,
    // identifier
    Identifier,
    // enclosed in double quotes
    DoubleQuotedValue,
    // enclosed in single quotes
    SingleQuotesValue,
    // :
    Colon,
    // ;
    Semicolon,
    // @
    At,
    // !important
    Important,
    // {
    LRbrace,
    // }
    RBrace,
    // (
    LParen,
    // )
    RParen,
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