public namespace md {

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
    ChemicalStart,  // ${ (Optional/Legacy but kept for lexer parity)
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
    Strikethrough,
    AutoLink,
    Superscript,
    Subscript,
    Insert,
    Mark,
    Footnote,
    FootnoteDef,
    DefinitionList,
    DefinitionTerm,
    DefinitionData,
    Abbreviation,
    CustomContainer,
    TaskCheckbox
}

public struct MdNode {
    var kind : MdNodeKind
}

public struct MdReference {
    var url : std::string_view
    var title : std::string_view
}

public struct MdRoot {
    var base : MdNode
    // Removed parent *ASTNode and support SymResSupport as they are compiler-specific
    // Children managed by Arena (pointers)
    var children : std::vector<*mut MdNode>
    // Removed dyn_values
    // reference_defs might still be useful for link resolution
    var reference_defs : std::unordered_map<std::string, MdReference>
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
    // Removed *mut Value, kept blank or string representation if needed?
    // Since we don't evaluate, maybe we just store the raw text representation
    var text : std::string_view
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
    var title : std::string_view
    var children : std::vector<*mut MdNode>
}

public struct MdAutoLink {
    var base : MdNode
    var url : std::string_view
}

public struct MdImage {
    var base : MdNode
    var url : std::string_view
    var alt : std::string_view
    var title : std::string_view
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
    var start : int
    var children : std::vector<*mut MdNode>
}

public struct MdListItem {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdTaskCheckbox {
    var base : MdNode
    var checked : bool
}

public enum MdTableAlign {
    Left,
    Center,
    Right,
    None
}

public struct MdTable {
    var base : MdNode
    var alignments : std::vector<MdTableAlign>
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

public struct MdSuperscript {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdSubscript {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdInsert {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdMark {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdFootnote {
    var base : MdNode
    var id : std::string_view
}

public struct MdFootnoteDef {
    var base : MdNode
    var id : std::string_view
    var children : std::vector<*mut MdNode>
}

public struct MdDefinitionList {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdDefinitionTerm {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdDefinitionData {
    var base : MdNode
    var children : std::vector<*mut MdNode>
}

public struct MdAbbreviation {
    var base : MdNode
    var id : std::string_view
    var title : std::string_view
}

public struct MdCustomContainer {
    var base : MdNode
    var type : std::string_view
    var children : std::vector<*mut MdNode>
}

} // namespace md
