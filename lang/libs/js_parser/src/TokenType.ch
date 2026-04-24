public enum JsTokenType {
    EndOfFile = 30000,
    Var = 100,
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
    StrictEqual,
    NotEqual,
    StrictNotEqual,
    LessThan,
    GreaterThan,
    LessThanEqual,
    GreaterThanEqual,
    Exclamation,
    Arrow, // =>
    Const,
    Let,
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
    Debugger,
    Regex,
    JSXText
}

public func isIdOrKw(type : JsTokenType) : bool {
    switch(type) {
        JsTokenType.Identifier, JsTokenType.Function, JsTokenType.Return, JsTokenType.If, JsTokenType.Else, JsTokenType.Const, JsTokenType.Let, JsTokenType.For, JsTokenType.While,
        JsTokenType.Break, JsTokenType.Continue, JsTokenType.Switch, JsTokenType.Case, JsTokenType.Default, JsTokenType.Do,
        JsTokenType.Try, JsTokenType.Catch, JsTokenType.Finally, JsTokenType.Throw,
        JsTokenType.True, JsTokenType.False, JsTokenType.Null, JsTokenType.Undefined,
        JsTokenType.Typeof, JsTokenType.Void, JsTokenType.Delete, JsTokenType.In, JsTokenType.InstanceOf,
        JsTokenType.New, JsTokenType.Of, JsTokenType.This, JsTokenType.Async, JsTokenType.Await, JsTokenType.Class, JsTokenType.Extends,
        JsTokenType.Super, JsTokenType.Static, JsTokenType.Import, JsTokenType.Export, JsTokenType.Yield, JsTokenType.Debugger => return true;
        default => return false
    }
}