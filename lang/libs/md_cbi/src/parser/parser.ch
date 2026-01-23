public struct MdParser {
    var dyn_values : *mut std::vector<*mut Value>
}

func isHashToken(token_type : int) : bool {
    return token_type == MdTokenType.Hash as int;
}

func isNewlineToken(token_type : int) : bool {
    return token_type == MdTokenType.Newline as int;
}

func isGreaterThanToken(token_type : int) : bool {
    return token_type == MdTokenType.GreaterThan as int;
}

func isDashToken(token_type : int) : bool {
    return token_type == MdTokenType.Dash as int;
}

func isPlusToken(token_type : int) : bool {
    return token_type == MdTokenType.Plus as int;
}

func isBacktickToken(token_type : int) : bool {
    return token_type == MdTokenType.Backtick as int;
}

func isStarToken(token_type : int) : bool {
    return token_type == MdTokenType.Star as int;
}

func isRBraceToken(token_type : int) : bool {
    return token_type == MdTokenType.RBrace as int;
}

func isEndOfFileToken(token_type : int) : bool {
    return token_type == MdTokenType.EndOfFile as int;
}

func isTextToken(token_type : int) : bool {
    return token_type == MdTokenType.Text as int;
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
    
    while(!isRBraceToken(parser.getToken().type) && !isEndOfFileToken(parser.getToken().type)) {
        var node = mdParser.parseBlockNode(parser, builder);
        if(node != null) {
            root.children.push(node);
        }
    }
    
    return root;
}

func (md : &mut MdParser) parseBlockNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    const token = parser.getToken();
    const tok_type = token.type;
    
    // Skip leading whitespace-only text tokens
    if(isTextToken(tok_type)) {
        const txt = token.value;
        var all_ws = true;
        var i = 0u;
        while(i < txt.size()) {
            const c = txt.data()[i];
            if(c != ' ' && c != '\t' && c != '\r') {
                all_ws = false;
                break;
            }
            i++;
        }
        if(all_ws) {
            parser.increment();
            return null;
        }
    }
    
    // Use explicit if-else for reliable token matching
    if(isHashToken(tok_type)) {
        return md.parseHeader(parser, builder);
    }
    
    if(isNewlineToken(tok_type)) {
        parser.increment();
        return null;
    }
    
    if(isGreaterThanToken(tok_type)) {
        return md.parseBlockquote(parser, builder);
    }
    
    if(isDashToken(tok_type) || isPlusToken(tok_type) || isStarToken(tok_type)) {
        // Check if it's a horizontal rule (---, ***, ___)
        if(md.isHorizontalRule(parser)) {
            return md.parseHorizontalRule(parser, builder);
        }
        // Check if it's a list
        if(isDashToken(tok_type) || isPlusToken(tok_type)) {
            return md.parseList(parser, builder, false);
        }
    }
    
    if(isBacktickToken(tok_type)) {
        // Check for fenced code block (```)
        if(md.isFencedCodeBlock(parser)) {
            return md.parseCodeBlock(parser, builder);
        }
    }
    
    // Default: paragraph
    return md.parseParagraph(parser, builder);
}

func (md : &mut MdParser) isHorizontalRule(parser : *mut Parser) : bool {
    // A horizontal rule is 3+ of the same character (-, *, _) on a line
    // For simplicity, check if we have 3+ dashes/stars in a row
    const tok_type = parser.getToken().type;
    if(!isDashToken(tok_type) && !isStarToken(tok_type)) {
        return false;
    }
    // Would need lookahead to properly detect, for now return false
    return false; 
}

func (md : &mut MdParser) isFencedCodeBlock(parser : *mut Parser) : bool {
    // Check for ``` at start
    // Would need lookahead
    return false;
}

func (md : &mut MdParser) parseHeader(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var level = 0;
    while(isHashToken(parser.getToken().type)) {
        level++;
        parser.increment();
    }
    
    // Skip space after hashes
    if(isTextToken(parser.getToken().type)) {
        const txt = parser.getToken().value;
        if(txt.size() > 0 && txt.data()[0] == ' ') {
            // Trim leading space by creating a new view
            var new_view = std::string_view(txt.data() + 1, txt.size() - 1);
            // We'll handle this in the inline parsing
        }
    }
    
    var header = builder.allocate<MdHeader>()
    new (header) MdHeader {
        base : MdNode { kind : MdNodeKind.Header },
        level : level,
        children : std::vector<*mut MdNode>()
    }
    
    // Read until newline or macro end
    while(!isNewlineToken(parser.getToken().type) && 
          !isRBraceToken(parser.getToken().type) && 
          !isEndOfFileToken(parser.getToken().type)) {
        var node = md.parseInlineNode(parser, builder);
        if(node != null) header.children.push(node);
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    return header as *mut MdNode;
}

func (md : &mut MdParser) parseBlockquote(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    parser.increment(); // consume >
    
    var bq = builder.allocate<MdBlockquote>()
    new (bq) MdBlockquote {
        base : MdNode { kind : MdNodeKind.Blockquote },
        children : std::vector<*mut MdNode>()
    }
    
    // Parse content until newline
    while(!isNewlineToken(parser.getToken().type) && 
          !isRBraceToken(parser.getToken().type) && 
          !isEndOfFileToken(parser.getToken().type)) {
        var node = md.parseInlineNode(parser, builder);
        if(node != null) bq.children.push(node);
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    return bq as *mut MdNode;
}

func (md : &mut MdParser) parseHorizontalRule(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    // Consume 3+ dashes/stars
    while(isDashToken(parser.getToken().type) || isStarToken(parser.getToken().type)) {
        parser.increment();
    }
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    var hr = builder.allocate<MdHorizontalRule>()
    new (hr) MdHorizontalRule {
        base : MdNode { kind : MdNodeKind.Hr }
    }
    return hr as *mut MdNode;
}

func (md : &mut MdParser) parseList(parser : *mut Parser, builder : *mut ASTBuilder, ordered : bool) : *mut MdNode {
    var list = builder.allocate<MdList>()
    new (list) MdList {
        base : MdNode { kind : MdNodeKind.List },
        ordered : ordered,
        children : std::vector<*mut MdNode>()
    }
    
    while(isDashToken(parser.getToken().type) || isPlusToken(parser.getToken().type)) {
        parser.increment(); // consume - or +
        
        var item = builder.allocate<MdListItem>()
        new (item) MdListItem {
            base : MdNode { kind : MdNodeKind.ListItem },
            children : std::vector<*mut MdNode>()
        }
        
        // Parse until newline
        while(!isNewlineToken(parser.getToken().type) && 
              !isRBraceToken(parser.getToken().type) && 
              !isEndOfFileToken(parser.getToken().type)) {
            var node = md.parseInlineNode(parser, builder);
            if(node != null) item.children.push(node);
        }
        
        list.children.push(item as *mut MdNode);
        
        if(isNewlineToken(parser.getToken().type)) {
            parser.increment();
        }
        
        // Check if next line is also a list item
        // Skip whitespace
        if(isTextToken(parser.getToken().type)) {
            const txt = parser.getToken().value;
            var all_ws = true;
            var j = 0u;
            while(j < txt.size()) {
                if(txt.data()[j] != ' ' && txt.data()[j] != '\t') {
                    all_ws = false;
                    break;
                }
                j++;
            }
            if(all_ws) {
                parser.increment();
            }
        }
    }
    
    return list as *mut MdNode;
}

func (md : &mut MdParser) parseCodeBlock(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    // Consume opening ```
    parser.increment(); // first `
    parser.increment(); // second `
    parser.increment(); // third `
    
    var lang = std::string_view("");
    if(isTextToken(parser.getToken().type)) {
        lang = parser.getToken().value;
        parser.increment();
    }
    
    // Skip newline after language
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    // Collect code until closing ```
    // For simplicity, just collect text until we hit ```
    var code_str = std::string_view("");
    // This is simplified - would need proper implementation
    
    var cb = builder.allocate<MdCodeBlock>()
    new (cb) MdCodeBlock {
        base : MdNode { kind : MdNodeKind.CodeBlock },
        language : lang,
        code : code_str
    }
    
    return cb as *mut MdNode;
}

func (md : &mut MdParser) parseParagraph(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var para = builder.allocate<MdParagraph>()
    new (para) MdParagraph {
        base : MdNode { kind : MdNodeKind.Paragraph },
        children : std::vector<*mut MdNode>()
    }
    
    // Read until double newline or macro end
    while(!isRBraceToken(parser.getToken().type) && 
          !isEndOfFileToken(parser.getToken().type)) {
        
        // Check for double newline
        if(isNewlineToken(parser.getToken().type)) {
             parser.increment();
             if(isNewlineToken(parser.getToken().type)) {
                 parser.increment();
                 break;
             }
             // Single newline - check if next is a block element
             if(isHashToken(parser.getToken().type) || 
                isGreaterThanToken(parser.getToken().type)) {
                 break; // Let block parser handle it
             }
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
    const tok_type = token.type;
    
    if(isTextToken(tok_type)) {
        var text = builder.allocate<MdText>()
        new (text) MdText {
            base : MdNode { kind : MdNodeKind.Text },
            value : builder.allocate_view(token.value)
        }
        parser.increment();
        return text as *mut MdNode;
    }
    
    if(tok_type == MdTokenType.ChemicalStart as int) {
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
    
    if(isStarToken(tok_type)) {
        return md.parseBoldOrItalic(parser, builder);
    }
    
    if(tok_type == MdTokenType.Underscore as int) {
        return md.parseUnderscoreEmphasis(parser, builder);
    }
    
    if(tok_type == MdTokenType.Tilde as int) {
        return md.parseStrikethrough(parser, builder);
    }
    
    if(tok_type == MdTokenType.LBracket as int) {
        return md.parseLink(parser, builder);
    }
    
    if(tok_type == MdTokenType.Exclamation as int) {
        return md.parseImage(parser, builder);
    }
    
    if(isBacktickToken(tok_type)) {
        return md.parseInlineCode(parser, builder);
    }
    
    // Default: treat as text
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : builder.allocate_view(token.value)
    }
    parser.increment();
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseBoldOrItalic(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var count = 0;
    while(isStarToken(parser.getToken().type)) {
        count++;
        parser.increment();
    }
    
    if(count == 1) {
        // Italic
        var italic = builder.allocate<MdItalic>()
        new (italic) MdItalic {
            base : MdNode { kind : MdNodeKind.Italic },
            children : std::vector<*mut MdNode>()
        }
        while(!isStarToken(parser.getToken().type) && 
              !isNewlineToken(parser.getToken().type) && 
              !isRBraceToken(parser.getToken().type) && 
              !isEndOfFileToken(parser.getToken().type)) {
            var child = md.parseInlineNode(parser, builder);
            if(child != null) italic.children.push(child);
        }
        if(isStarToken(parser.getToken().type)) parser.increment();
        return italic as *mut MdNode;
    } else {
        // Bold (2+ stars)
        var bold = builder.allocate<MdBold>()
        new (bold) MdBold {
            base : MdNode { kind : MdNodeKind.Bold },
            children : std::vector<*mut MdNode>()
        }
        while(true) {
            const t = parser.getToken();
            if(isEndOfFileToken(t.type) || isRBraceToken(t.type) || isNewlineToken(t.type)) {
                break;
            }
            if(isStarToken(t.type)) {
                parser.increment();
                if(isStarToken(parser.getToken().type)) {
                    parser.increment();
                    break; // Found closing **
                }
                // Single star inside bold
                var starText = builder.allocate<MdText>()
                new (starText) MdText {
                    base : MdNode { kind : MdNodeKind.Text },
                    value : std::string_view("*")
                }
                bold.children.push(starText as *mut MdNode);
                continue;
            }
            var child = md.parseInlineNode(parser, builder);
            if(child != null) bold.children.push(child);
        }
        return bold as *mut MdNode;
    }
}

func (md : &mut MdParser) parseUnderscoreEmphasis(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    var count = 0;
    while(parser.getToken().type == MdTokenType.Underscore as int) {
        count++;
        parser.increment();
    }
    
    if(count == 1) {
        var italic = builder.allocate<MdItalic>()
        new (italic) MdItalic {
            base : MdNode { kind : MdNodeKind.Italic },
            children : std::vector<*mut MdNode>()
        }
        while(parser.getToken().type != MdTokenType.Underscore as int && 
              !isNewlineToken(parser.getToken().type) && 
              !isRBraceToken(parser.getToken().type) && 
              !isEndOfFileToken(parser.getToken().type)) {
            var child = md.parseInlineNode(parser, builder);
            if(child != null) italic.children.push(child);
        }
        if(parser.getToken().type == MdTokenType.Underscore as int) parser.increment();
        return italic as *mut MdNode;
    } else {
        var bold = builder.allocate<MdBold>()
        new (bold) MdBold {
            base : MdNode { kind : MdNodeKind.Bold },
            children : std::vector<*mut MdNode>()
        }
        while(true) {
            const t = parser.getToken();
            if(isEndOfFileToken(t.type) || isRBraceToken(t.type) || isNewlineToken(t.type)) {
                break;
            }
            if(t.type == MdTokenType.Underscore as int) {
                parser.increment();
                if(parser.getToken().type == MdTokenType.Underscore as int) {
                    parser.increment();
                    break;
                }
                var usText = builder.allocate<MdText>()
                new (usText) MdText {
                    base : MdNode { kind : MdNodeKind.Text },
                    value : std::string_view("_")
                }
                bold.children.push(usText as *mut MdNode);
                continue;
            }
            var child = md.parseInlineNode(parser, builder);
            if(child != null) bold.children.push(child);
        }
        return bold as *mut MdNode;
    }
}

func (md : &mut MdParser) parseStrikethrough(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    // Expect ~~
    parser.increment(); // first ~
    if(parser.getToken().type == MdTokenType.Tilde as int) {
        parser.increment(); // second ~
    }
    
    var strike = builder.allocate<MdStrikethrough>()
    new (strike) MdStrikethrough {
        base : MdNode { kind : MdNodeKind.Strikethrough },
        children : std::vector<*mut MdNode>()
    }
    
    while(true) {
        const t = parser.getToken();
        if(isEndOfFileToken(t.type) || isRBraceToken(t.type) || isNewlineToken(t.type)) {
            break;
        }
        if(t.type == MdTokenType.Tilde as int) {
            parser.increment();
            if(parser.getToken().type == MdTokenType.Tilde as int) {
                parser.increment();
                break; // Found closing ~~
            }
            var tildeText = builder.allocate<MdText>()
            new (tildeText) MdText {
                base : MdNode { kind : MdNodeKind.Text },
                value : std::string_view("~")
            }
            strike.children.push(tildeText as *mut MdNode);
            continue;
        }
        var child = md.parseInlineNode(parser, builder);
        if(child != null) strike.children.push(child);
    }
    
    return strike as *mut MdNode;
}

func (md : &mut MdParser) parseLink(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    parser.increment(); // consume [
    var text_children = std::vector<*mut MdNode>();
    
    while(parser.getToken().type != MdTokenType.RBracket as int && 
          !isNewlineToken(parser.getToken().type) && 
          !isRBraceToken(parser.getToken().type) && 
          !isEndOfFileToken(parser.getToken().type)) {
        var child = md.parseInlineNode(parser, builder);
        if(child != null) text_children.push(child);
    }
    
    if(parser.getToken().type == MdTokenType.RBracket as int) {
        parser.increment();
        if(parser.getToken().type == MdTokenType.LParen as int) {
            parser.increment();
            var url = std::string_view("");
            if(isTextToken(parser.getToken().type)) {
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
    
    // Fallback
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : std::string_view("[")
    }
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseImage(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    parser.increment(); // consume !
    
    if(parser.getToken().type == MdTokenType.LBracket as int) {
        parser.increment(); // consume [
        var alt = std::string_view("");
        if(isTextToken(parser.getToken().type)) {
            alt = builder.allocate_view(parser.getToken().value);
            parser.increment();
        }
        if(parser.increment_if(MdTokenType.RBracket as int)) {
            if(parser.increment_if(MdTokenType.LParen as int)) {
                var url = std::string_view("");
                if(isTextToken(parser.getToken().type)) {
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
        var text = builder.allocate<MdText>()
        new (text) MdText {
            base : MdNode { kind : MdNodeKind.Text },
            value : std::string_view("![")
        }
        return text as *mut MdNode;
    }
    
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : std::string_view("!")
    }
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseInlineCode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut MdNode {
    parser.increment(); // consume `
    var value = std::string_view("");
    if(isTextToken(parser.getToken().type)) {
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
    
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : std::string_view("`")
    }
    return text as *mut MdNode;
}
