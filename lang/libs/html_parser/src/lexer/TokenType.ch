public enum TokenType {

    EndOfFile,
    Unexpected,

    TagName,
    AttrName,

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