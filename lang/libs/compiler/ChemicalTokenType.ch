enum ChemicalTokenType {

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
    ImportKw,
    FuncKw,
    TypealiasKw,
    StructKw,
    UnionKw,
    VariantKw,
    InterfaceKw,
    ImplKw,
    NamespaceKw,
    EnumKw,
    VarKw,
    UsingKw,
    ComptimeKw,
    MutKw,
    SelfKw,
    ThisKw,
    AsKw,
    IsKw,
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

    // when the value is inside double quotes
    String,
    // when the value is inside single quotes
    Char,
    // #macro_name
    HashMacro,
    // @annotation
    Annotation,

    // escape sequence inside quotes
    EscapeSeq,

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

    // '
    SingleQuoteSym,
    // "
    DoubleQuoteSym,

    // -------- END OPERATORS ------

    // a number, doesn't include the negative sign
    Number

}