public enum MdTokenType {
    EndOfFile = 30000,
    Text = 100,
    Hash,           // #
    Star,           // *
    Underscore,     // _
    LBracket,       // [
    RBracket,       // ]
    LParen,         // (
    RParen,         // )
    LBrace,         // {
    RBrace,         // }
    Exclamation,    // !
    Backtick,       // `
    GreaterThan,    // >
    Dash,           // -
    Plus,           // +
    Pipe,           // |
    Newline,        // \n
    ChemicalStart,  // ${
    Tilde,          // ~
    Colon,          // :
    EndMd,          // #endmd
    FencedCodeStart,// ``` or ```lang
    FencedCodeEnd,  // ```
    CodeContent,    // raw code inside fenced block
    Number,         // for ordered lists like 1. 2. etc
    Dot,            // .
    Equal,          // =
    Caret,          // ^
}