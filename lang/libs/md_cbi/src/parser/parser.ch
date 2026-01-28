public struct MdParser {
    var dyn_values : *mut std::vector<*mut Value>
    var builder : *mut ASTBuilder
}

// Helper to peek at next token without consuming
func peek_token(parser : *mut Parser) : *mut Token {
    const ptr = parser.getTokenPtr();
    return (*ptr) + 1;
}

// Helper to peek at token at offset i from current
func peek_token_at(parser : *mut Parser, i : int) : *mut Token {
    const ptr = parser.getTokenPtr();
    return (*ptr) + i;
}

func isEndMdToken(t : int) : bool { return t == MdTokenType.EndMd as int; }
func isHashToken(t : int) : bool { return t == MdTokenType.Hash as int; }
func isNewlineToken(t : int) : bool { return t == MdTokenType.Newline as int; }
func isGreaterThanToken(t : int) : bool { return t == MdTokenType.GreaterThan as int; }
func isDashToken(t : int) : bool { return t == MdTokenType.Dash as int; }
func isPlusToken(t : int) : bool { return t == MdTokenType.Plus as int; }
func isStarToken(t : int) : bool { return t == MdTokenType.Star as int; }
func isUnderscoreToken(t : int) : bool { return t == MdTokenType.Underscore as int; }
func isBacktickToken(t : int) : bool { return t == MdTokenType.Backtick as int; }
func isEndOfFileToken(t : int) : bool { return t == MdTokenType.EndOfFile as int; }
func isTextToken(t : int) : bool { return t == MdTokenType.Text as int; }
func isColonToken(t : int) : bool { return t == MdTokenType.Colon as int; }
func isPipeToken(t : int) : bool { return t == MdTokenType.Pipe as int; }
func isNumberToken(t : int) : bool { return t == MdTokenType.Number as int; }
func isDotToken(t : int) : bool { return t == MdTokenType.Dot as int; }
func isFencedCodeStartToken(t : int) : bool { return t == MdTokenType.FencedCodeStart as int; }
func isFencedCodeEndToken(t : int) : bool { return t == MdTokenType.FencedCodeEnd as int; }
func isCodeContentToken(t : int) : bool { return t == MdTokenType.CodeContent as int; }
func isTildeToken(t : int) : bool { return t == MdTokenType.Tilde as int; }
func isLBracketToken(t : int) : bool { return t == MdTokenType.LBracket as int; }
func isExclamationToken(t : int) : bool { return t == MdTokenType.Exclamation as int; }
func isChemicalStartToken(t : int) : bool { return t == MdTokenType.ChemicalStart as int; }
func isRBraceToken(t : int) : bool { return t == MdTokenType.RBrace as int; }
func isRBracketToken(t : int) : bool { return t == MdTokenType.RBracket as int; }
func isCaretToken(t : int) : bool { return t == MdTokenType.Caret as int; }
func isEqualToken(t : int) : bool { return t == MdTokenType.Equal as int; }

func isBlockEnd(t : int) : bool {
    return isEndMdToken(t) || isEndOfFileToken(t);
}

func isLineEnd(t : int) : bool {
    return isNewlineToken(t) || isBlockEnd(t);
}

func isListStart(parser : *mut Parser) : bool {
    const t = parser.getToken().type;
    if(isDashToken(t) || isPlusToken(t) || isStarToken(t)) {
        const next = peek_token(parser);
        return isTextToken(next.type) && next.value.size() > 0 && next.value.data()[0] == ' ';
    }
    return false;
}

func isOrderedListStart(parser : *mut Parser) : bool {
    const t = parser.getToken().type;
    if(isNumberToken(t)) {
        const next = peek_token(parser);
        if(isDotToken(next.type)) {
             const next_next = peek_token_at(parser, 2);
             return isTextToken(next_next.type) && next_next.value.size() > 0 && next_next.value.data()[0] == ' ';
        }
    }
    return false;
}

func isBlockStart(parser : *mut Parser) : bool {
    const t = parser.getToken().type;
    if(isHashToken(t) || isFencedCodeStartToken(t) || isGreaterThanToken(t) || isPipeToken(t) || isUnderscoreToken(t)) {
        return true;
    }
    if(isListStart(parser)) return true;
    if(isOrderedListStart(parser)) return true;
    
    // Check for Footnote Definition: [^id]:
    if(isLBracketToken(t)) {
        const next = peek_token(parser);
        if(isCaretToken(next.type)) {
            // Further check for ]:
            var i = 2;
            while(true) {
                const tok = peek_token_at(parser, i);
                if(isRBracketToken(tok.type)) {
                    const next2 = peek_token_at(parser, i + 1);
                    if(isColonToken(next2.type)) return true;
                    break;
                }
                if(isLineEnd(tok.type)) break;
                i++;
            }
        }
    }
    
    // Check for Abbreviation: *[id]:
    if(isStarToken(t)) {
        const next = peek_token(parser);
        if(isLBracketToken(next.type)) {
             var i = 2;
            while(true) {
                const tok = peek_token_at(parser, i);
                if(isRBracketToken(tok.type)) {
                    const next2 = peek_token_at(parser, i + 1);
                    if(isColonToken(next2.type)) return true;
                    break;
                }
                if(isLineEnd(tok.type)) break;
                i++;
            }
        }
    }
    
    // Check for Custom Container: :::
    if(isColonToken(t)) {
        const t2 = peek_token(parser).type;
        const t3 = peek_token_at(parser, 2).type;
        if(isColonToken(t2) && isColonToken(t3)) return true;
        // Also check for definition list start: ":" followed by space
        const t2_val = peek_token(parser).value;
        if(isTextToken(t2) && t2_val.size() > 0 && t2_val.data()[0] == ' ') return true;
    }
    
    // Check for HR: *** or ---
    if(isStarToken(t) || isDashToken(t)) {
        // Check for 3+
        var count = 0;
        var i = 0;
        while(true) {
            const tok = peek_token_at(parser, i);
            if(tok.type == t) {
                count++;
                i++;
            } else if(isTextToken(tok.type) && isWhitespaceOnlyText(tok.value)) {
                i++;
            } else if(isNewlineToken(tok.type) || isBlockEnd(tok.type)) {
                break;
            } else {
                return false; 
            }
        }
        return count >= 3;
    }
    return false;
}

func isWhitespaceOnlyText(txt : std::string_view) : bool {
    var i = 0u;
    while(i < txt.size()) {
        const c = txt.data()[i];
        if(c != ' ' && c != '\t' && c != '\r') {
            return false;
        }
        i++;
    }
    return true;
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
    
    var mdParser = MdParser { dyn_values : &mut root.dyn_values, builder : builder }
    
    while(!isBlockEnd(parser.getToken().type)) {
        var node = mdParser.parseBlockNode(parser);
        if(node != null) {
            root.children.push(node);
        }
    }
    
    return root;
}

func (md : &mut MdParser) parseBlockNode(parser : *mut Parser) : *mut MdNode {
    const token = parser.getToken();
    const t = token.type;
    
    // Skip whitespace-only text
    if(isTextToken(t) && isWhitespaceOnlyText(token.value)) {
        parser.increment();
        return null;
    }
    
    // Newline - skip
    if(isNewlineToken(t)) {
        parser.increment();
        return null;
    }
    
    // Header: # ## ### etc
    if(isHashToken(t)) {
        return md.parseHeader(parser);
    }
    
    // Fenced code block: ```
    if(isFencedCodeStartToken(t)) {
        return md.parseFencedCodeBlock(parser);
    }
    
    // Blockquote: >
    if(isGreaterThanToken(t)) {
        return md.parseBlockquote(parser);
    }
    
    // Unordered list: - + *

    if(isDashToken(t) || isPlusToken(t) || isStarToken(t)) {
        if(isListStart(parser)) {
             return md.parseList(parser, false, 1);
        }
        
        if(isStarToken(t) || isDashToken(t)) {
             if(md.isHorizontalRule(parser)) {
                 return md.parseHorizontalRule(parser);
             }
        }
    }
    
    // Check for horizontal rule: ___ (Underscore)
    if(isUnderscoreToken(t)) {
        if(md.isHorizontalRule(parser)) {
            return md.parseHorizontalRule(parser);
        }
    }
    
    // Ordered list: 1. 2. etc
    if(isOrderedListStart(parser)) {
        return md.parseOrderedList(parser);
    }
    
    // Table: |
    if(isPipeToken(t)) {
        return md.parseTable(parser);
    }
    
    // Footnote Definition: [^id]:
    if(isLBracketToken(t)) {
        if(isCaretToken(peek_token(parser).type)) {
            return md.parseFootnoteDef(parser);
        }
    }
    
    // Abbreviation: *[id]:
    if(isStarToken(t)) {
        if(isLBracketToken(peek_token(parser).type)) {
            return md.parseAbbreviation(parser);
        }
    }
    
    // Custom Container or Definition List
    if(isColonToken(t)) {
        if(isColonToken(peek_token(parser).type) && isColonToken(peek_token_at(parser, 2).type)) {
            return md.parseCustomContainer(parser);
        }
        // Definition List
        return md.parseDefinitionList(parser);
    }
    
    // Default: paragraph
    return md.parseParagraph(parser);
}

func (md : &mut MdParser) isHorizontalRule(parser : *mut Parser) : bool {
    // Look for 3+ of same character (*, -, _)
    const first_type = parser.getToken().type;
    var count = 0;
    var i = 0;
    while(true) {
        const tok = peek_token_at(parser, i);
        const t = tok.type;
        if(t == first_type) {
            count++;
            i++;
        } else if(isTextToken(t) && isWhitespaceOnlyText(tok.value)) {
            i++;
        } else if(isNewlineToken(t) || isBlockEnd(t)) {
            break;
        } else {
            return false;
        }
    }
    return count >= 3;
}

func (md : &mut MdParser) parseHorizontalRule(parser : *mut Parser) : *mut MdNode {
    // Consume all dashes/stars/underscores until newline
    while(!isLineEnd(parser.getToken().type)) {
        parser.increment();
    }
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    var builder = md.builder;
    var hr = builder.allocate<MdHorizontalRule>()
    new (hr) MdHorizontalRule {
        base : MdNode { kind : MdNodeKind.Hr }
    }
    return hr as *mut MdNode;
}

func (md : &mut MdParser) parseHeader(parser : *mut Parser) : *mut MdNode {
    var level = 0;
    while(isHashToken(parser.getToken().type)) {
        level++;
        parser.increment();
    }
    if(level > 6) level = 6;

    var builder = md.builder;
    var header = builder.allocate<MdHeader>()
    new (header) MdHeader {
        base : MdNode { kind : MdNodeKind.Header },
        level : level,
        children : std::vector<*mut MdNode>()
    }
    
    while(!isLineEnd(parser.getToken().type)) {
        var node = md.parseInlineNode(parser);
        if(node != null) header.children.push(node);
    }

    // Skip space after hashes
    // Skip leading spaces in the first text token if present
    if(header.children.size() > 0) {
        var first = header.children.get(0);
        if(first.kind == MdNodeKind.Text) {
            var txtNode = first as *mut MdText;
            if(txtNode.value.size() > 0 && txtNode.value.data()[0] == ' ') {
                // Determine how many spaces
                var spaces = 0u;
                while(spaces < txtNode.value.size() && txtNode.value.data()[spaces] == ' ') {
                    spaces++;
                }
                if(spaces < txtNode.value.size()) {
                    // Update view to skip spaces
                     txtNode.value = std::string_view(txtNode.value.data() + spaces, txtNode.value.size() - spaces);
                } else {
                    // All spaces, better to just remove this node? For now just empty it
                    txtNode.value = std::string_view("");
                }
            }
        }
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    return header as *mut MdNode;
}

func (md : &mut MdParser) parseFencedCodeBlock(parser : *mut Parser) : *mut MdNode {
    const lang = parser.getToken().value;
    parser.increment(); // consume FencedCodeStart
    
    var code = std::string();
    
    while(!isFencedCodeEndToken(parser.getToken().type) && !isBlockEnd(parser.getToken().type)) {
        if(isCodeContentToken(parser.getToken().type)) {
            code.append_view(parser.getToken().value);
            code.append('\n');
        }
        parser.increment();
    }
    
    if(isFencedCodeEndToken(parser.getToken().type)) {
        parser.increment();
    }

    var builder = md.builder;
    var cb = builder.allocate<MdCodeBlock>()
    new (cb) MdCodeBlock {
        base : MdNode { kind : MdNodeKind.CodeBlock },
        language : md.builder.allocate_view(lang),
        code : md.builder.allocate_view(code.to_view())
    }
    return cb as *mut MdNode;
}

func (md : &mut MdParser) parseBlockquote(parser : *mut Parser) : *mut MdNode {
    var depth = 0;
    while(isGreaterThanToken(parser.getToken().type)) {
        depth++;
        parser.increment();
        // Skip space after matching >
        if(isTextToken(parser.getToken().type) && parser.getToken().value.size() > 0 && parser.getToken().value.data()[0] == ' ') {
             const txt = parser.getToken().value;
             // Only consume if it's a single space, or effectively if we want to skip 1 char?
             // Simple approach: if it starts with space, consume one space logic is implied by loop
             // But tokens are chunks.
             // If token is just " ", consume it.
             if(txt.size() == 1) {
                 parser.increment();
             }
        }
    }
    
    var builder = md.builder;
    var root_bq = builder.allocate<MdBlockquote>();
    new (root_bq) MdBlockquote {
        base : MdNode { kind : MdNodeKind.Blockquote },
        children : std::vector<*mut MdNode>()
    }
    
    var current_container = root_bq;
    var d = 1;
    while(d < depth) {
         var nested = builder.allocate<MdBlockquote>();
         new (nested) MdBlockquote {
             base : MdNode { kind : MdNodeKind.Blockquote },
             children : std::vector<*mut MdNode>()
         }
         current_container.children.push(nested as *mut MdNode);
         current_container = nested;
         d++;
    }
    
    // Parse content until newline
    while(!isLineEnd(parser.getToken().type)) {
        var node = md.parseInlineNode(parser);
        if(node != null) current_container.children.push(node);
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    return root_bq as *mut MdNode;
}

func (md : &mut MdParser) parseList(parser : *mut Parser, ordered : bool, start : int) : *mut MdNode {

    var builder = md.builder;
    var list = builder.allocate<MdList>()
    new (list) MdList {
        base : MdNode { kind : MdNodeKind.List },
        ordered : ordered,
        start : start,
        children : std::vector<*mut MdNode>()
    }
    
    while(isDashToken(parser.getToken().type) || isPlusToken(parser.getToken().type) || isStarToken(parser.getToken().type)) {
        parser.increment(); // consume marker
        
        // Skip space
        if(isTextToken(parser.getToken().type) && parser.getToken().value.size() > 0 && parser.getToken().value.data()[0] == ' ') {
            // Will handle in inline
        }
        
        var item = builder.allocate<MdListItem>()
        new (item) MdListItem {
            base : MdNode { kind : MdNodeKind.ListItem },
            children : std::vector<*mut MdNode>()
        }
        
        while(!isLineEnd(parser.getToken().type)) {
            var node = md.parseInlineNode(parser);
            if(node != null) item.children.push(node);
        }
        
        list.children.push(item as *mut MdNode);
        
        if(isNewlineToken(parser.getToken().type)) {
            parser.increment();
        }
        
        // Skip whitespace
        if(isTextToken(parser.getToken().type) && isWhitespaceOnlyText(parser.getToken().value)) {
            parser.increment();
        }
    }
    
    return list as *mut MdNode;
}

func (md : &mut MdParser) parseOrderedList(parser : *mut Parser) : *mut MdNode {
    // Get start number
    var start_num = 1;
    if(isNumberToken(parser.getToken().type)) {
        // Parse number
        const num_str = parser.getToken().value;
        start_num = 0;
        var i = 0u;
        while(i < num_str.size()) {
            start_num = start_num * 10 + (num_str.data()[i] - '0') as int;
            i++;
        }
    }
    
    var builder = md.builder;
    var list = builder.allocate<MdList>()
    new (list) MdList {
        base : MdNode { kind : MdNodeKind.List },
        ordered : true,
        start : start_num,
        children : std::vector<*mut MdNode>()
    }
    
    while(isNumberToken(parser.getToken().type)) {
        // Check sequences: Number -> Dot -> Space
        const next = peek_token(parser);
        if(!isDotToken(next.type)) {
            break;
        }
        
        parser.increment(); // consume number
        parser.increment(); // consume .
        
        // Skip space
        if(isTextToken(parser.getToken().type) && parser.getToken().value.size() > 0 && parser.getToken().value.data()[0] == ' ') {
            // Will handle in inline
        }
        
        var item = builder.allocate<MdListItem>()
        new (item) MdListItem {
            base : MdNode { kind : MdNodeKind.ListItem },
            children : std::vector<*mut MdNode>()
        }
        
        while(!isLineEnd(parser.getToken().type)) {
            var node = md.parseInlineNode(parser);
            if(node != null) item.children.push(node);
        }
        
        list.children.push(item as *mut MdNode);
        
        if(isNewlineToken(parser.getToken().type)) {
            parser.increment();
        }
        
        // Skip whitespace lines
        while(isTextToken(parser.getToken().type) && isWhitespaceOnlyText(parser.getToken().value)) {
            parser.increment();
        }
        if(isNewlineToken(parser.getToken().type)) {
             parser.increment();
        }
    }
    
    return list as *mut MdNode;
}


func (md : &mut MdParser) parseTable(parser : *mut Parser) : *mut MdNode {
    var builder = md.builder;
    var table = builder.allocate<MdTable>()
    new (table) MdTable {
        base : MdNode { kind : MdNodeKind.Table },
        alignments : std::vector<MdTableAlign>(),
        children : std::vector<*mut MdNode>()
    }
    
    // Parse header row
    var header_row = md.parseTableRow(parser, true);
    if(header_row != null) {
        table.children.push(header_row);
    }
    
    // Check for alignment row
    if(isPipeToken(parser.getToken().type)) {
        md.parseTableAlignmentRow(parser, table);
    }
    
    // Parse data rows
    while(isPipeToken(parser.getToken().type)) {
        var row = md.parseTableRow(parser, false);
        if(row != null) {
            table.children.push(row);
        }
    }
    
    return table as *mut MdNode;
}

func (md : &mut MdParser) parseTableRow(parser : *mut Parser, is_header : bool) : *mut MdNode {
    var builder = md.builder;
    var row = builder.allocate<MdTableRow>()
    new (row) MdTableRow {
        base : MdNode { kind : MdNodeKind.TableRow },
        is_header : is_header,
        children : std::vector<*mut MdNode>()
    }
    
    // Skip leading |
    if(isPipeToken(parser.getToken().type)) {
        parser.increment();
    }
    
    while(!isLineEnd(parser.getToken().type)) {
        var cell = builder.allocate<MdTableCell>()
        new (cell) MdTableCell {
            base : MdNode { kind : MdNodeKind.TableCell },
            children : std::vector<*mut MdNode>()
        }
        
        // Parse cell content until | or newline
        while(!isPipeToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
            var node = md.parseInlineNode(parser);
            if(node != null) cell.children.push(node);
        }
        
        row.children.push(cell as *mut MdNode);
        
        if(isPipeToken(parser.getToken().type)) {
            parser.increment();
            // Check if next is newline - if so we are done with row, don't create empty cell
            if(isLineEnd(parser.getToken().type)) {
                break;
            }
        }
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
    
    return row as *mut MdNode;
}

func (md : &mut MdParser) parseTableAlignmentRow(parser : *mut Parser, table : *mut MdTable) {
    // Skip |
    if(isPipeToken(parser.getToken().type)) {
        parser.increment();
    }
    
    while(!isLineEnd(parser.getToken().type)) {
        var align = MdTableAlign.None;
        var has_left_colon = false;
        var has_right_colon = false;
        
        // Check for : at start
        if(parser.getToken().type == MdTokenType.Colon as int) {
            has_left_colon = true;
            parser.increment();
        }
        
        // Skip dashes
        while(isDashToken(parser.getToken().type)) {
            parser.increment();
        }
        
        // Check for : at end
        if(parser.getToken().type == MdTokenType.Colon as int) {
            has_right_colon = true;
            parser.increment();
        }
        
        if(has_left_colon && has_right_colon) {
            align = MdTableAlign.Center;
        } else if(has_left_colon) {
            align = MdTableAlign.Left;
        } else if(has_right_colon) {
            align = MdTableAlign.Right;
        }
        
        table.alignments.push(align);
        
        // Skip whitespace
        if(isTextToken(parser.getToken().type) && isWhitespaceOnlyText(parser.getToken().value)) {
            parser.increment();
        }
        
        if(isPipeToken(parser.getToken().type)) {
            parser.increment();
        }
    }
    
    if(isNewlineToken(parser.getToken().type)) {
        parser.increment();
    }
}

func (md : &mut MdParser) parseParagraph(parser : *mut Parser) : *mut MdNode {
    var builder = md.builder;
    var para = builder.allocate<MdParagraph>()
    new (para) MdParagraph {
        base : MdNode { kind : MdNodeKind.Paragraph },
        children : std::vector<*mut MdNode>()
    }
    
    while(!isBlockEnd(parser.getToken().type)) {
        if(isNewlineToken(parser.getToken().type)) {
            parser.increment();
            // Check for double newline or block element
            if(isNewlineToken(parser.getToken().type) || isBlockStart(parser)) {
                break;
            }
            // Single newline - add space
            continue;
        }
        
        var node = md.parseInlineNode(parser);
        if(node != null) para.children.push(node);
    }
    
    if(para.children.size() == 0) return null;
    return para as *mut MdNode;
}

func (md : &mut MdParser) parseInlineNode(parser : *mut Parser) : *mut MdNode {
    var builder = md.builder;
    const token = parser.getToken();
    const t = token.type;
    
    if(isTextToken(t)) {
        var text = builder.allocate<MdText>()
        new (text) MdText {
            base : MdNode { kind : MdNodeKind.Text },
            value : md.builder.allocate_view(token.value)
        }
        parser.increment();
        return text as *mut MdNode;
    }
    
    if(isChemicalStartToken(t)) {
        parser.increment();
        var val = parser.parseExpression(md.builder);
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
    
    if(isStarToken(t)) {
        return md.parseBoldOrItalic(parser, '*');
    }
    
    if(isUnderscoreToken(t)) {
        return md.parseBoldOrItalic(parser, '_');
    }
    
    if(isTildeToken(t)) {
        if(isTildeToken(peek_token(parser).type)) {
            return md.parseStrikethrough(parser);
        } else {
            return md.parseSubscript(parser);
        }
    }
    
    if(isCaretToken(t)) {
        return md.parseSuperscript(parser);
    }
    
    if(isEqualToken(t)) {
        if(isEqualToken(peek_token(parser).type)) {
            return md.parseMark(parser);
        }
    }
    
    if(isPlusToken(t)) {
        if(isPlusToken(peek_token(parser).type)) {
            return md.parseInsert(parser);
        }
    }
    
    if(isLBracketToken(t)) {
        if(isCaretToken(peek_token(parser).type)) {
            return md.parseFootnote(parser);
        }
        return md.parseLink(parser);
    }
    
    if(isExclamationToken(t)) {
        return md.parseImage(parser);
    }
    
    if(isBacktickToken(t)) {
        return md.parseInlineCode(parser);
    }
    
    // Default: treat token value as text
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : builder.allocate_view(token.value)
    }
    parser.increment();
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseBoldOrItalic(parser : *mut Parser, marker : char) : *mut MdNode {
    const marker_type = if(marker == '*') MdTokenType.Star as int else MdTokenType.Underscore as int;
    var builder = md.builder;
    var count = 0;
    while(parser.getToken().type == marker_type) {
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
        while(parser.getToken().type != marker_type && !isLineEnd(parser.getToken().type)) {
            var child = md.parseInlineNode(parser);
            if(child != null) italic.children.push(child);
        }
        if(parser.getToken().type == marker_type) parser.increment();
        return italic as *mut MdNode;
    } else {
        // Bold (2+ markers)
        var bold = builder.allocate<MdBold>()
        new (bold) MdBold {
            base : MdNode { kind : MdNodeKind.Bold },
            children : std::vector<*mut MdNode>()
        }
        while(true) {
            if(isLineEnd(parser.getToken().type)) break;
            if(parser.getToken().type == marker_type) {
                parser.increment();
                if(parser.getToken().type == marker_type) {
                    parser.increment();
                    break;
                }
                var markerText = builder.allocate<MdText>()
                new (markerText) MdText {
                    base : MdNode { kind : MdNodeKind.Text },
                    value : if(marker == '*') std::string_view("*") else std::string_view("_")
                }
                bold.children.push(markerText as *mut MdNode);
                continue;
            }
            var child = md.parseInlineNode(parser);
            if(child != null) bold.children.push(child);
        }
        return bold as *mut MdNode;
    }
}

func (md : &mut MdParser) parseStrikethrough(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // first ~
    parser.increment(); // second ~

    var builder = md.builder;
    var strike = builder.allocate<MdStrikethrough>()
    new (strike) MdStrikethrough {
        base : MdNode { kind : MdNodeKind.Strikethrough },
        children : std::vector<*mut MdNode>()
    }
    
    while(true) {
        if(isLineEnd(parser.getToken().type)) break;
        if(isTildeToken(parser.getToken().type) && isTildeToken(peek_token(parser).type)) {
            parser.increment();
            parser.increment();
            break;
        }
        var child = md.parseInlineNode(parser);
        if(child != null) strike.children.push(child);
    }
    
    return strike as *mut MdNode;
}

func (md : &mut MdParser) parseSubscript(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // ~
    var builder = md.builder;
    var sub = builder.allocate<MdSubscript>()
    new (sub) MdSubscript {
        base : MdNode { kind : MdNodeKind.Subscript },
        children : std::vector<*mut MdNode>()
    }
    while(!isTildeToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
        var child = md.parseInlineNode(parser);
        if(child != null) sub.children.push(child);
    }
    if(isTildeToken(parser.getToken().type)) parser.increment();
    return sub as *mut MdNode;
}

func (md : &mut MdParser) parseSuperscript(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // ^
    var builder = md.builder;
    var sup = builder.allocate<MdSuperscript>()
    new (sup) MdSuperscript {
        base : MdNode { kind : MdNodeKind.Superscript },
        children : std::vector<*mut MdNode>()
    }
    while(!isCaretToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
        var child = md.parseInlineNode(parser);
        if(child != null) sup.children.push(child);
    }
    if(isCaretToken(parser.getToken().type)) parser.increment();
    return sup as *mut MdNode;
}

func (md : &mut MdParser) parseMark(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // =
    parser.increment(); // =
    var builder = md.builder;
    var mark = builder.allocate<MdMark>()
    new (mark) MdMark {
        base : MdNode { kind : MdNodeKind.Mark },
        children : std::vector<*mut MdNode>()
    }
    while(true) {
        if(isLineEnd(parser.getToken().type)) break;
        if(isEqualToken(parser.getToken().type) && isEqualToken(peek_token(parser).type)) {
            parser.increment();
            parser.increment();
            break;
        }
        var child = md.parseInlineNode(parser);
        if(child != null) mark.children.push(child);
    }
    return mark as *mut MdNode;
}

func (md : &mut MdParser) parseInsert(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // +
    parser.increment(); // +
    var builder = md.builder;
    var ins = builder.allocate<MdInsert>()
    new (ins) MdInsert {
        base : MdNode { kind : MdNodeKind.Insert },
        children : std::vector<*mut MdNode>()
    }
    while(true) {
        if(isLineEnd(parser.getToken().type)) break;
        if(isPlusToken(parser.getToken().type) && isPlusToken(peek_token(parser).type)) {
            parser.increment();
            parser.increment();
            break;
        }
        var child = md.parseInlineNode(parser);
        if(child != null) ins.children.push(child);
    }
    return ins as *mut MdNode;
}

func (md : &mut MdParser) parseFootnote(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // [
    parser.increment(); // ^
    var id = std::string();
    while(!isRBracketToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
        id.append_view(parser.getToken().value)
        parser.increment();
    }
    if(isRBracketToken(parser.getToken().type)) parser.increment();
    
    var builder = md.builder;
    var fn = builder.allocate<MdFootnote>()
    new (fn) MdFootnote {
        base : MdNode { kind : MdNodeKind.Footnote },
        id : builder.allocate_view(id.to_view())
    }
    return fn as *mut MdNode;
}

func (md : &mut MdParser) parseFootnoteDef(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // [
    parser.increment(); // ^
    var id = std::string();
    while(!isRBracketToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
        id.append_view(parser.getToken().value);
        parser.increment();
    }
    if(isRBracketToken(parser.getToken().type)) parser.increment();
    if(isColonToken(parser.getToken().type)) parser.increment();
    
    var builder = md.builder;
    var fn = builder.allocate<MdFootnoteDef>()
    new (fn) MdFootnoteDef {
        base : MdNode { kind : MdNodeKind.FootnoteDef },
        id : builder.allocate_view(id.to_view()),
        children : std::vector<*mut MdNode>()
    }
    
    // Parse rest of line/block as children
    while(!isNewlineToken(parser.getToken().type) && !isBlockEnd(parser.getToken().type)) {
        var node = md.parseInlineNode(parser);
        if(node != null) fn.children.push(node);
    }
    
    return fn as *mut MdNode;
}

func (md : &mut MdParser) parseAbbreviation(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // *
    parser.increment(); // [
    var id = std::string();
    while(!isRBracketToken(parser.getToken().type) && !isLineEnd(parser.getToken().type)) {
        id.append_view(parser.getToken().value);
        parser.increment();
    }
    if(isRBracketToken(parser.getToken().type)) parser.increment();
    if(isColonToken(parser.getToken().type)) parser.increment();
    
    // Skip whitespace
    if(isTextToken(parser.getToken().type) && isWhitespaceOnlyText(parser.getToken().value)) {
        parser.increment();
    }
    
    var title = std::string();
    while(!isLineEnd(parser.getToken().type)) {
        title.append_view(parser.getToken().value);
        parser.increment();
    }
    
    var builder = md.builder;
    var abb = builder.allocate<MdAbbreviation>()
    new (abb) MdAbbreviation {
        base : MdNode { kind : MdNodeKind.Abbreviation },
        id : builder.allocate_view(id.to_view()),
        title : builder.allocate_view(title.to_view())
    }
    return abb as *mut MdNode;
}

func (md : &mut MdParser) parseCustomContainer(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // :
    parser.increment(); // :
    parser.increment(); // :
    
    var builder = md.builder;
    
    // Read type
    var type_view = std::string_view();
    if(isTextToken(parser.getToken().type)) {
        type_view = parser.getToken().value;
        parser.increment();
    }
    
    // Skip to next line
    while(!isNewlineToken(parser.getToken().type) && !isBlockEnd(parser.getToken().type)) {
        parser.increment();
    }
    if(isNewlineToken(parser.getToken().type)) parser.increment();
    
    var container = builder.allocate<MdCustomContainer>()
    new (container) MdCustomContainer {
        base : MdNode { kind : MdNodeKind.CustomContainer },
        type : builder.allocate_view(type_view),
        children : std::vector<*mut MdNode>()
    }
    
    // Read until ::: closing
    while(!isBlockEnd(parser.getToken().type)) {
        const t = parser.getToken().type;
        if(isColonToken(t)) {
            if(isColonToken(peek_token(parser).type) && isColonToken(peek_token_at(parser, 2).type)) {
                 parser.increment();
                 parser.increment();
                 parser.increment();
                 break;
            }
        }
        var node = md.parseBlockNode(parser);
        if(node != null) container.children.push(node);
    }
    
    return container as *mut MdNode;
}

func (md : &mut MdParser) parseDefinitionList(parser : *mut Parser) : *mut MdNode {
    var builder = md.builder;
    var dl = builder.allocate<MdDefinitionList>()
    new (dl) MdDefinitionList {
        base : MdNode { kind : MdNodeKind.DefinitionList },
        children : std::vector<*mut MdNode>()
    }
    
    // Current token is : (start of definition data)
    while(!isBlockEnd(parser.getToken().type)) {
        if(isColonToken(parser.getToken().type)) {
            parser.increment(); // :
            // Skip space
            if(isTextToken(parser.getToken().type) && parser.getToken().value.size() > 0 && parser.getToken().value.data()[0] == ' ') {
                // Actually need to handle the space specifically if it's part of the text token
                // For now just assume it's there
            }
            
            var dd = builder.allocate<MdDefinitionData>()
            new (dd) MdDefinitionData {
                base : MdNode { kind : MdNodeKind.DefinitionData },
                children : std::vector<*mut MdNode>()
            }
            while(!isLineEnd(parser.getToken().type)) {
                var node = md.parseInlineNode(parser);
                if(node != null) dd.children.push(node);
            }
            dl.children.push(dd as *mut MdNode);
            if(isNewlineToken(parser.getToken().type)) parser.increment();
        } else {
             // If not : maybe another term or break
             break;
        }
    }
    
    return dl as *mut MdNode;
}

func (md : &mut MdParser) parseUrl(parser : *mut Parser) : std::string_view {
    var url_res = std::string();
    var builder = md.builder;
    
    while(true) {
        const t = parser.getToken().type;
        if(isTextToken(t) || isColonToken(t) || isDotToken(t) || isDashToken(t) || isPlusToken(t) || isUnderscoreToken(t) || isNumberToken(t) || isHashToken(t) || isPipeToken(t)) {
             url_res.append_view(parser.getToken().value);
             parser.increment();
             continue;
        }
        
        if(t == MdTokenType.RParen as int || t == MdTokenType.RBracket as int || isLineEnd(t) || (isTextToken(t) && parser.getToken().value.data()[0] == ' ')) {
            break;
        }
        
        if(isEndMdToken(t) || t == MdTokenType.RBrace as int) break;
        
        // Default consume
         url_res.append_view(parser.getToken().value);
         parser.increment();
    }
    
    return builder.allocate_view(url_res.to_view());
}

func (md : &mut MdParser) parseLink(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // consume [
    var text_children = std::vector<*mut MdNode>();


    while(parser.getToken().type != MdTokenType.RBracket as int && !isLineEnd(parser.getToken().type)) {
        var child = md.parseInlineNode(parser);
        if(child != null) text_children.push(child);
    }

    var builder = md.builder;
    
    if(parser.getToken().type == MdTokenType.RBracket as int) {
        parser.increment();
        if(parser.getToken().type == MdTokenType.LParen as int) {
            parser.increment();
            
            var url = md.parseUrl(parser);
            var title = std::string_view("");
            
            // Check for title
            if(isTextToken(parser.getToken().type)) {
                const txt = parser.getToken().value;
                 // Loop to skip spaces
                if(txt.size() > 0 && txt.data()[0] == ' ') {
                     parser.increment();
                     // Now expect title
                     if(isTextToken(parser.getToken().type)) {
                         const ttxt = parser.getToken().value;
                         if(ttxt.size() >= 2 && ttxt.data()[0] == '"') {
                             // Strip quotes if they exist at both ends check
                             // Lexer probably includes them? Yes usually text token. 
                             // But wait, if text token is "Title", it has quotes.
                             if(ttxt.data()[ttxt.size()-1] == '"') {
                                 title = builder.allocate_view(std::string_view(ttxt.data() + 1, ttxt.size() - 2));
                             } else {
                                 title = builder.allocate_view(ttxt);
                             }
                             parser.increment();
                         }
                     }
                }
            }
            
            if(parser.getToken().type == MdTokenType.RParen as int) {
                parser.increment();
                var link = builder.allocate<MdLink>()
                new (link) MdLink {
                    base : MdNode { kind : MdNodeKind.Link },
                    url : url,
                    title : title,
                    children : text_children
                }
                return link as *mut MdNode;
            }
        }
    }
    
    var text = builder.allocate<MdText>()
    new (text) MdText {
        base : MdNode { kind : MdNodeKind.Text },
        value : std::string_view("[")
    }
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseImage(parser : *mut Parser) : *mut MdNode {
    parser.increment(); // consume !
    var builder = md.builder;
    if(isLBracketToken(parser.getToken().type)) {
        parser.increment(); // consume [
        
        var alt = std::string_view("");
        if(isTextToken(parser.getToken().type)) {
            alt = md.builder.allocate_view(parser.getToken().value);
            parser.increment();
        }
        
        if(parser.increment_if(MdTokenType.RBracket as int)) {
            if(parser.increment_if(MdTokenType.LParen as int)) {
                var url = md.parseUrl(parser);
                var title = std::string_view("");
                
                 if(isTextToken(parser.getToken().type)) {
                    const txt = parser.getToken().value;
                     if(txt.size() > 0 && txt.data()[0] == ' ') {
                         parser.increment();
                         if(isTextToken(parser.getToken().type)) {
                             const ttxt = parser.getToken().value;
                             if(ttxt.size() >= 2 && ttxt.data()[0] == '"') {
                                 if(ttxt.data()[ttxt.size()-1] == '"') {
                                     title = builder.allocate_view(std::string_view(ttxt.data() + 1, ttxt.size() - 2));
                                 } else {
                                     title = builder.allocate_view(ttxt);
                                 }
                                 parser.increment();
                             }
                         }
                    }
                }
                
                if(parser.increment_if(MdTokenType.RParen as int)) {
                    var img = builder.allocate<MdImage>()
                    new (img) MdImage {
                        base : MdNode { kind : MdNodeKind.Image },
                        url : url,
                        alt : alt,
                        title : title
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

func (md : &mut MdParser) parseInlineCode(parser : *mut Parser) : *mut MdNode {
    var builder = md.builder;
    parser.increment(); // consume `
    
    var value = std::string_view("");
    if(isTextToken(parser.getToken().type)) {
        value = md.builder.allocate_view(parser.getToken().value);
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
