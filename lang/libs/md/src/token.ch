public namespace md {

public enum MdTokenType {
    EndOfFile,
    Text = 100,
    Hash,
    Star,
    Underscore,
    LBracket,
    RBracket,
    LParen,
    RParen,
    Exclamation,
    Backtick,
    GreaterThan,
    Dash,
    Plus,
    Pipe,
    Newline,
    Tilde,
    Colon,
    Number,
    Dot,
    Equal,
    Caret,
    FencedCodeStart,
    FencedCodeEnd,
    CodeContent,
    LBrace,
    RBrace,
}

public struct MdToken {
    var type : int
    var value : std::string_view
    var position : size_t
}

}
