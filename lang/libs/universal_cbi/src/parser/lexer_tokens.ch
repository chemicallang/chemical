public enum JsTokenType {
    EndOfFile,
    Var = 100,
    JSXText,
    Identifier,
    Equal,
    Number,
    LBrace,
    RBrace,
    ChemicalStart, // ${
    LParen,
    RParen,
    LBracket,
    RBracket,
    SemiColon,
    String,
    Function,
    Return,
    If,
    Else,
    Comma,
    Colon,
    Dot,
    Plus,
    Minus,
    Star,
    Slash,
    EqualEqual,
    NotEqual,
    LessThan,
    GreaterThan,
    LessThanEqual,
    GreaterThanEqual,
    Exclamation,
    Arrow, // =>
    Const,
    Let,
    State,
    For,
    While,
    LogicalAnd,      // &&
    LogicalOr,       // ||
    Question,        // ?
    PlusPlus,        // ++
    MinusMinus,      // --
    Break,
    Continue,
    Switch,
    Case,
    Default,
    Do,
    Try,
    Catch,
    Finally,
    Throw,
    TemplateLiteral,  // `string`
    True,
    False,
    Null,
    Undefined,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    Typeof,
    Void,
    Delete,
    In,
    InstanceOf,
    BitwiseNot, // ~
    BitwiseAnd, // &
    BitwiseOr,  // |
    BitwiseXor, // ^
    LeftShift,  // <<
    RightShift, // >>
    RightShiftUnsigned, // >>>
    New,
    Of,
    This,
    ThreeDots, // ...
    Async,
    Await,
    Class,
    Extends,
    Super,
    Static,
    Import,
    Export,
    Yield,
    Debugger
}
