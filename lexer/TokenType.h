// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class TokenType {

    // when reached the end of file
    EndOfFile,

    // usually means an error should occur
    Unexpected,

    // ------- keywords
    ForKw,
    SwitchKw,
    LoopKw,
    ReturnKw,
    BreakKw,
    NewKw,
    ContinueKw,
    DestructKw,
    DeleteKw,
    DeallocKw,
    ProvideKw,
    DefaultKw,
    UnsafeKw,
    UnreachableKw,
    InitKw,
    TryKw,
    CatchKw,
    ThrowKw,
    IfKw,
    FromKw,
    ElseKw,
    WhileKw,
    DoKw,
    TrueKw,
    FalseKw,
    NullKw,
    PublicKw,
    PrivateKw,
    ProtectedKw,
    InternalKw,
    CharKw,
    ShortKw,
    IntKw,
    LongKw,
    BigintKw,
    UCharKw,
    UShortKw,
    UIntKw,
    ULongKw,
    UBigintKw,
    BoolKw,
    AnyKw,
    DoubleKw,
    LongdoubleKw,
    FloatKw,
    Int128Kw,
    Uint128Kw,
    Float128Kw,
    VoidKw,
    SizeOfKw,
    AlignOfKw,
    ImportKw,
    FuncKw,
    TypeKw,
    StructKw,
    UnionKw,
    VariantKw,
    InterfaceKw,
    ImplKw,
    NamespaceKw,
    EnumKw,
    AliasKw,
    VarKw,
    UsingKw,
    ComptimeKw,
    MutKw,
    SelfKw,
    ThisKw,
    AsKw,
    IsKw,
    InKw,
    DynKw,
    ConstKw,

    // -------

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

    // when the value is inside single quotes, for example 'x' or even '\x1b'
    // escape sequences will be processed
    Char,
    // when the value is inside double quotes
    String,
    // a multiline string begins and ends with three double quotes
    MultilineString,
    // #macro_name
    HashMacro,
    // @annotation
    Annotation,

    // when this text is part of a single line comment
    SingleLineComment,

    // when this text is part of a multi line comment
    MultiLineComment,

    // ---------
    // Symbols
    // ---------

    // +
    PlusSym,
    // -
    MinusSym,
    // *
    MultiplySym,
    // /
    DivideSym,
    // %
    ModSym,

    // ++
    DoublePlusSym,
    // --
    DoubleMinusSym,

    // =
    EqualSym,
    // ==
    DoubleEqualSym,
    // !=
    NotEqualSym,
    // <=
    LessThanOrEqualSym,
    // <
    LessThanSym,
    // >=
    GreaterThanOrEqualSym,
    // >
    GreaterThanSym,

    // &&
    LogicalAndSym,
    // ||
    LogicalOrSym,

    // <<
    LeftShiftSym,
    // >>
    RightShiftSym,

    // &
    AmpersandSym,
    // |
    PipeSym,
    // ^
    CaretUpSym,
    // ~
    BitNotSym,

    // :
    ColonSym,
    // ::
    DoubleColonSym,
    // !
    NotSym,
    // .
    DotSym,
    // ,
    CommaSym,
    // ;
    SemiColonSym,

    // ...
    // TODO remove this
    TripleDotSym,

    // =>
    LambdaSym,

    // -------- END OPERATORS ------

    // a number, doesn't include the negative sign
    Number,

    // an expression inside string starts with the following token
    // "this is here -> {expr}"
    // it's a '{', where the end is '}' but we call these explicitly expr start and end
    // so not to mistake between them during parsing
    StringExprStart,
    StringExprEnd,

    IndexKwStart = ForKw,
    IndexKwEnd = ConstKw,



};