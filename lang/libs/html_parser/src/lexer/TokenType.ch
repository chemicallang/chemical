public enum TokenType {

    EndOfFile,
    Unexpected,

    TagName,
    AttrName,

    Text,
    Number,

    LessThan,
    GreaterThan,

    // </
    TagEnd,

    LBrace,
    RBrace,

    Equal,
    SingleQuotedValue,
    DoubleQuotedValue,

    FwdSlash,

    // <!DOC
    DeclarationStart,
    // <!--
    CommentStart,
    CommentText,


}