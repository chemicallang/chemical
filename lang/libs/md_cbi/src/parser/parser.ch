public enum MdTokenType {
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
    EndOfFile,
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

public struct MdParser {
    var dyn_values : *mut std::vector<*mut Value>
}

public func parseMdRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdRoot {
    var root = builder.allocate<MdRoot>()
    new (root) MdRoot {
        base : MdNode { kind : MdNodeKind.Root },
        parent : parser.getParentNode(),
        children : std::vector<*mut MdNode>(),
        dyn_values : std::vector<*mut Value>(),
        support : SymResSupport {}
    }
    
    var mdParser = MdParser { dyn_values : &mut root.dyn_values }
    
    while(parser.getToken().type != MdTokenType.RBrace as int && parser.getToken().type != MdTokenType.EndOfFile as int) {
        var node = mdParser.parseBlockNode(parser, builder);
        if(node != null) {
            root.children.push(node);
        }
    }
    
    return root;
}

func (md : &mut MdParser) parseBlockNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    const token = parser.getToken();
    
    switch(token.type) {
        MdTokenType.Hash as int => {
            return md.parseHeader(parser, builder);
        }
        MdTokenType.Newline as int => {
            parser.increment();
            return null;
        }
        default => {
            return md.parseParagraph(parser, builder);
        }
    }
}

func (md : &mut MdParser) parseHeader(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var level = 0;
    while(parser.getToken().type == MdTokenType.Hash as int) {
        level++;
        parser.increment();
    }
    
    var header = builder.allocate<MdHeader>()
    new (header) MdHeader {
        base : MdNode { kind : MdNodeKind.Header },
        level : level,
        children : std::vector<*mut MdNode>()
    }
    
    // Read until newline or macro end
    while(parser.getToken().type != MdTokenType.Newline as int && 
          parser.getToken().type != MdTokenType.RBrace as int && 
          parser.getToken().type != MdTokenType.EndOfFile as int) {
        var node = md.parseInlineNode(parser, builder);
        if(node != null) header.children.push(node);
    }
    
    if(parser.getToken().type == MdTokenType.Newline as int) {
        parser.increment();
    }
    
    return header as *mut MdNode;
}

func (md : &mut MdParser) parseParagraph(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var para = builder.allocate<MdParagraph>()
    new (para) MdParagraph {
        base : MdNode { kind : MdNodeKind.Paragraph },
        children : std::vector<*mut MdNode>()
    }
    
    // Read until double newline or macro end
    while(parser.getToken().type != MdTokenType.RBrace as int && 
          parser.getToken().type != MdTokenType.EndOfFile as int) {
        
        // Check for double newline (simple implementation)
        if(parser.getToken().type == MdTokenType.Newline as int) {
             parser.increment();
             if(parser.getToken().type == MdTokenType.Newline as int) {
                 parser.increment();
                 break; // End of paragraph
             }
             // Single newline in paragraph is just a space usually in MD, 
             // but let's just add it as text or ignore for now.
             continue;
        }
        
        var node = md.parseInlineNode(parser, builder);
        if(node != null) para.children.push(node);
    }
    
    if(para.children.size() == 0) return null;
    return para as *mut MdNode;
}

func (md : &mut MdParser) parseInlineNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    const token = parser.getToken();
    
    switch(token.type) {
        MdTokenType.Text as int => {
            var text = builder.allocate<MdText>()
            new (text) MdText {
                base : MdNode { kind : MdNodeKind.Text },
                value : builder.allocate_view(token.value)
            }
            parser.increment();
            return text as *mut MdNode;
        }
        MdTokenType.ChemicalStart as int => {
             parser.increment();
             var val = parser.parseExpression(builder);
             if(val == null) {
                 parser.error("expected a chemical expression");
             } else {
                 md.dyn_values.push(val)
             }
             if(!parser.increment_if(MdTokenType.RBrace as int)) {
                 parser.error("expected } after chemical value");
             }
             var interp = builder.allocate<MdInterpolation>()
             new (interp) MdInterpolation {
                 base : MdNode { kind : MdNodeKind.Interpolation },
                 value : val
             }
             return interp as *mut MdNode;
        }
        MdTokenType.Star as int => {
            var count = 0;
            while(parser.getToken().type == MdTokenType.Star as int) {
                count++;
                parser.increment();
            }
            
            if(count == 1) { // Italic
                var italic = builder.allocate<MdItalic>()
                new (italic) MdItalic {
                    base : MdNode { kind : MdNodeKind.Italic },
                    children : std::vector<*mut MdNode>()
                }
                while(parser.getToken().type != MdTokenType.Star as int && 
                      parser.getToken().type != MdTokenType.Newline as int && 
                      parser.getToken().type != MdTokenType.RBrace as int && 
                      parser.getToken().type != MdTokenType.EndOfFile as int) {
                    var child = md.parseInlineNode(parser, builder);
                    if(child != null) italic.children.push(child);
                }
                if(parser.getToken().type == MdTokenType.Star as int) parser.increment();
                return italic as *mut MdNode;
            } else if(count >= 2) { // Bold
                var bold = builder.allocate<MdBold>()
                new (bold) MdBold {
                    base : MdNode { kind : MdNodeKind.Bold },
                    children : std::vector<*mut MdNode>()
                }
                const t1 = parser.getToken()
                while(!(t1.type == MdTokenType.Star as int &&t1.type == MdTokenType.Star as int) &&
                      t1.type != MdTokenType.Newline as int &&
                      t1.type != MdTokenType.RBrace as int &&
                      t1.type != MdTokenType.EndOfFile as int) {
                    var child = md.parseInlineNode(parser, builder);
                    if(child != null) bold.children.push(child);
                }
                if(parser.getToken().type == MdTokenType.Star as int) parser.increment();
                if(parser.getToken().type == MdTokenType.Star as int) parser.increment();
                return bold as *mut MdNode;
            }
            return null;
        }
        MdTokenType.LBracket as int => {
            // [text](url)
            parser.increment();
            var text_children = std::vector<*mut MdNode>();
            while(parser.getToken().type != MdTokenType.RBracket as int && 
                  parser.getToken().type != MdTokenType.Newline as int && 
                  parser.getToken().type != MdTokenType.RBrace as int && 
                  parser.getToken().type != MdTokenType.EndOfFile as int) {
                var child = md.parseInlineNode(parser, builder);
                if(child != null) text_children.push(child);
            }
            if(parser.getToken().type == MdTokenType.RBracket as int) {
                parser.increment();
                if(parser.getToken().type == MdTokenType.LParen as int) {
                    parser.increment();
                    var url = std::string_view("");
                    if(parser.getToken().type == MdTokenType.Text as int) {
                        url = builder.allocate_view(parser.getToken().value);
                        parser.increment();
                    }
                    if(parser.getToken().type == MdTokenType.RParen as int) {
                        parser.increment();
                        var link = builder.allocate<MdLink>()
                        new (link) MdLink {
                            base : MdNode { kind : MdNodeKind.Link },
                            url : url,
                            children : text_children
                        }
                        return link as *mut MdNode;
                    }
                }
            }
            // Fallback: If not a link, just return the bracket and children
            return null; // For now simplified
        }
        MdTokenType.Exclamation as int => {
            // ![alt](url)
            if(parser.getToken().type == MdTokenType.LBracket as int) {
                parser.increment(); // consume !
                parser.increment(); // consume [
                var alt = std::string_view("");
                if(parser.getToken().type == MdTokenType.Text as int) {
                    alt = builder.allocate_view(parser.getToken().value);
                    parser.increment();
                }
                if(parser.increment_if(MdTokenType.RBracket as int)) {
                    if(parser.increment_if(MdTokenType.LParen as int)) {
                        var url = std::string_view("");
                        if(parser.getToken().type == MdTokenType.Text as int) {
                            url = builder.allocate_view(parser.getToken().value);
                            parser.increment();
                        }
                        if(parser.increment_if(MdTokenType.RParen as int)) {
                            var img = builder.allocate<MdImage>()
                            new (img) MdImage {
                                base : MdNode { kind : MdNodeKind.Image },
                                url : url,
                                alt : alt
                            }
                            return img as *mut MdNode;
                        }
                    }
                }
                // fallback if not a full image syntax but starts with ![
                var text = builder.allocate<MdText>()
                new (text) MdText {
                    base : MdNode { kind : MdNodeKind.Text },
                    value : std::string_view("![")
                }
                return text as *mut MdNode;
            }
            // fallback
            var text = builder.allocate<MdText>()
            new (text) MdText {
                base : MdNode { kind : MdNodeKind.Text },
                value : std::string_view("!")
            }
            parser.increment();
            return text as *mut MdNode;
        }
        MdTokenType.Backtick as int => {
            parser.increment();
            var value = std::string_view("");
            if(parser.getToken().type == MdTokenType.Text as int) {
                value = builder.allocate_view(parser.getToken().value);
                parser.increment();
            }
            if(parser.increment_if(MdTokenType.Backtick as int)) {
                var code = builder.allocate<MdInlineCode>()
                new (code) MdInlineCode {
                    base : MdNode { kind : MdNodeKind.InlineCode },
                    value : value
                }
                return code as *mut MdNode;
            }
            // fallback
            var text = builder.allocate<MdText>()
            new (text) MdText {
                base : MdNode { kind : MdNodeKind.Text },
                value : std::string_view("`")
            }
            return text as *mut MdNode;
        }
        default => {
            var text = builder.allocate<MdText>()
            new (text) MdText {
                base : MdNode { kind : MdNodeKind.Text },
                value : builder.allocate_view(token.value)
            }
            parser.increment();
            return text as *mut MdNode;
        }
    }
}
