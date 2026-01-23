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
    Tilde,          // ~
    Colon,          // :
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
    TableCell,
    Strikethrough
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

public struct MdStrikethrough {
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

public struct MdCodeBlock {
    var base : MdNode
    var language : std::string_view
    var code : std::string_view
}

public struct MdBlockquote {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdHorizontalRule {
    var base : MdNode
}

public struct MdList {
    var base : MdNode
    var ordered : bool
    var children : std::vector<*mut MdNode>
}

public struct MdListItem {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdTable {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdTableRow {
    var base : MdNode
    var is_header : bool
    var children : std::vector<*mut MdNode>
}

public struct MdTableCell {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}
