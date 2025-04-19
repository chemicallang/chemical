enum TokenType {

    EndOfFile,
    Unexpected,

    Identifier,
    Text,
    Number,

    LessThan,
    GreaterThan,

    LBrace,
    RBrace,

    Equal,
    SingleQuotedValue,
    DoubleQuotedValue,

    FwdSlash,

    DeclarationStart, // <!DOC
    CommentStart, // <!--
    CommentText,


}