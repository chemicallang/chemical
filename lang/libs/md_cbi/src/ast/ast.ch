public enum MdTokenType {
    EndOfFile,
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
}

public enum MdNodeKind {
    Root,
    Header,
    Paragraph,
    List,
    ListItem,
    Text,
    Interpolation,
    Bold,
    Italic,
    Link,
    Image,
    CodeBlock,
    InlineCode,
    Blockquote,
    Hr,
    Table,
    TableRow,
    TableCell
}

public struct MdNode {
    var kind : MdNodeKind
}

public struct MdRoot {
    var base : MdNode
    var parent : *mut ASTNode
    var children : std::vector<*mut MdNode>
    var dyn_values : std::vector<*mut Value>
    var support : SymResSupport
}

public struct MdText {
    var base : MdNode
    var value : std::string_view
}

public struct MdHeader {
    var base : MdNode
    var level : int
    var children : std::vector<*mut MdNode>
}

public struct MdParagraph {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdInterpolation {
    var base : MdNode
    var value : *mut Value
}

public struct MdBold {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdItalic {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdLink {
    var base : MdNode
    var url : std::string_view
    var children : std::vector<*mut MdNode>
}

public struct MdImage {
    var base : MdNode
    var url : std::string_view
    var alt : std::string_view
}

public struct MdInlineCode {
    var base : MdNode
    var value : std::string_view
}
