public enum TokenType {
    EndOfFile = 30000,
    Unexpected = 30001,

    TagName = 100,
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

    // @{
    ChemicalNodeStart,

    // <!DOC
    DeclarationStart,
    // <!--
    CommentStart,
    CommentText,

    If,
    Else

}