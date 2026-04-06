public enum TokenType {
    EndOfFile = 30000,
    // unexpected
    Unexpected = 30001,
    // identifier
    Identifier = 100,
    // the class name selector present in global block, begins with '.'
    ClassName,
    // the id selector present in global block, begins with '#'
    Id,
    // property name is an identifier that appears in declaration
    PropertyName,
    // negative / positive, floating or non floating number
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
    // .
    Dot,
    // #
    Hash,
    // @
    At,
    // !important
    Important,
    // {
    LBrace,
    // ${
    DollarLBrace,
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
    // &
    Ampersand,
}