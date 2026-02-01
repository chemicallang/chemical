public namespace md {

public struct MdParser {
    var tokens : *std::vector<Token>
    var pos : size_t
    var arena : *mut Arena
    var root : *mut MdRoot
    var in_link : bool
}

// Helpers
func peek_token(parser : &mut MdParser) : Token {
    if(parser.pos + 1 >= parser.tokens.size()) return Token { type : MdTokenType.EndOfFile, value : std::string_view("") }
    return parser.tokens.get(parser.pos + 1)
}

func peek_token_at(parser : &mut MdParser, offset : size_t) : Token {
    if(parser.pos + offset >= parser.tokens.size()) return Token { type : MdTokenType.EndOfFile, value : std::string_view("") }
    return parser.tokens.get(parser.pos + offset)
}

func get_token(parser : &mut MdParser) : Token {
    if(parser.pos >= parser.tokens.size()) return Token { type : MdTokenType.EndOfFile, value : std::string_view("") }
    return parser.tokens.get(parser.pos)
}

func increment(parser : &mut MdParser) {
    if(parser.pos < parser.tokens.size()) parser.pos++;
}

// Token checks
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
func isLParenToken(t : int) : bool { return t == MdTokenType.LParen as int; }
func isRParenToken(t : int) : bool { return t == MdTokenType.RParen as int; }

func isBlockEnd(t : int) : bool {
    return isEndMdToken(t) || isEndOfFileToken(t);
}

func isLineEnd(t : int) : bool {
    return isNewlineToken(t) || isBlockEnd(t);
}

func isListStart(parser : &mut MdParser) : bool {
    const t = get_token(parser).type;
    if(isDashToken(t as int) || isPlusToken(t as int) || isStarToken(t as int)) {
        const next = peek_token(parser);
        return isTextToken(next.type as int) && next.value.size() > 0 && next.value.data()[0] == ' ';
    }
    return false;
}

func isOrderedListStart(parser : &mut MdParser) : bool {
    const t = get_token(parser).type;
    if(isNumberToken(t as int)) {
        const next = peek_token(parser);
        if(isDotToken(next.type as int)) {
             const next_next = peek_token_at(parser, 2);
             return isTextToken(next_next.type as int) && next_next.value.size() > 0 && next_next.value.data()[0] == ' ';
        }
    }
    return false;
}

func isBlockStart(parser : &mut MdParser) : bool {
    const t = get_token(parser).type;
    if(isHashToken(t as int) || isFencedCodeStartToken(t as int) || isGreaterThanToken(t as int) || isPipeToken(t as int) || isUnderscoreToken(t as int)) {
        return true;
    }
    if(isListStart(parser)) return true;
    if(isOrderedListStart(parser)) return true;
    
    // Check for Footnote Definition: [^id]:
    if(isLBracketToken(t as int)) {
        const next = peek_token(parser);
        if(isCaretToken(next.type as int)) {
            var i = 2u;
            while(true) {
                const tok = peek_token_at(parser, i);
                if(isRBracketToken(tok.type as int)) {
                    const next2 = peek_token_at(parser, i + 1u);
                    if(isColonToken(next2.type as int)) return true;
                    break;
                }
                if(isLineEnd(tok.type as int)) break;
                i++;
            }
        }
    }
    
    // Check for Abbreviation: *[id]:
    if(isStarToken(t as int)) {
        const next = peek_token(parser);
        if(isLBracketToken(next.type as int)) {
             var i = 2u;
            while(true) {
                const tok = peek_token_at(parser, i);
                if(isRBracketToken(tok.type as int)) {
                    const next2 = peek_token_at(parser, i + 1u);
                    if(isColonToken(next2.type as int)) return true;
                    break;
                }
                if(isLineEnd(tok.type as int)) break;
                i++;
            }
        }
    }
    
    // Check for Custom Container: :::
    if(isColonToken(t as int)) {
        const t2 = peek_token(parser).type;
        const t3 = peek_token_at(parser, 2).type;
        if(isColonToken(t2 as int) && isColonToken(t3 as int)) return true;
        // Definition list start: ":" followed by space
        const t2_val = peek_token(parser).value;
        if(isTextToken(t2 as int) && t2_val.size() > 0 && t2_val.data()[0] == ' ') return true;
    }
    
    // Check for HR: *** or ---
    if(isStarToken(t as int) || isDashToken(t as int)) {
        var count = 0;
        var i = 0u;
        while(true) {
            const tok = peek_token_at(parser, i);
            if(tok.type == t) {
                count++;
                i++;
            } else if(isTextToken(tok.type as int) && isWhitespaceOnlyText(tok.value)) {
                i++;
            } else if(isNewlineToken(tok.type as int) || isBlockEnd(tok.type as int)) {
                break;
            } else {
                return false; 
            }
        }
        return count >= 3;
    }
    
    // Reference Definition: [id]:
    if(isLBracketToken(t as int)) {
        var i = 1u;
        while(true) {
            const tok = peek_token_at(parser, i);
            if(isRBracketToken(tok.type as int)) {
                const next2 = peek_token_at(parser, i + 1u);
                if(isColonToken(next2.type as int)) return true;
                break;
            }
            if(isLineEnd(tok.type as int)) break;
            i++;
        }
    }
    
    return false;
}

func trim_view(view : std::string_view) : std::string_view {
    if(view.size() == 0) return view;
    var start = 0u;
    while(start < view.size() && (view.data()[start] == ' ' || view.data()[start] == '\t' || view.data()[start] == '\n' || view.data()[start] == '\r')) {
        start++;
    }
    if(start == view.size()) return std::string_view("");
    var end = view.size() - 1;
    while(end >= start && (view.data()[end] == ' ' || view.data()[end] == '\t' || view.data()[end] == '\n' || view.data()[end] == '\r')) {
        if(end == 0) break;
        end--;
    }
    return std::string_view(view.data() + start, end - start + 1);
}

func skipWhitespace(parser : &mut MdParser) {
    while(isTextToken(get_token(parser).type as int) && isWhitespaceOnlyText(get_token(parser).value)) {
        increment(parser);
    }
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

func consumeSingleLeadingSpace(parser : &mut MdParser) {
    if(isTextToken(get_token(parser).type as int) && isWhitespaceOnlyText(get_token(parser).value) && get_token(parser).value.size() > 0) {
        increment(parser);
    }
}

func (md : &mut MdParser) tryParseTaskCheckbox() : *mut MdNode {
    consumeSingleLeadingSpace(md);

    if(!isLBracketToken(get_token(md).type as int)) return null;

    const mid = peek_token(md);
    const endb = peek_token_at(md, 2);
    if(!isTextToken(mid.type as int) || mid.value.size() == 0) return null;
    if(!isRBracketToken(endb.type as int)) return null;

    if(mid.value.size() != 1) return null;
    const c = mid.value.data()[0];
    if(c != 'x' && c != 'X' && c != ' ') return null;

    increment(md); // [
    increment(md); // mid
    increment(md); // ]

    consumeSingleLeadingSpace(md);

    const arena = md.arena;
    var cb = arena.allocate<MdTaskCheckbox>();
    new (cb) MdTaskCheckbox {
        base : MdNode { kind : MdNodeKind.TaskCheckbox },
        checked : (c == 'x' || c == 'X')
    };
    return cb as *mut MdNode;
}

public func parse(tokens : *std::vector<Token>, arena : *mut Arena) : *mut MdRoot {
    const arena_block = arena;
    var root = arena_block.allocate<MdRoot>();
    new (root) MdRoot {
        base : MdNode { kind : MdNodeKind.Root },
        children : std::vector<*mut MdNode>(),
        reference_defs : std::unordered_map<std::string, MdReference>()
    }
    
    var mdParser = MdParser { tokens : tokens, pos : 0, arena : arena, root : root, in_link : false }
    
    while(!isBlockEnd(get_token(mdParser).type as int)) {
        var node = mdParser.parseBlockNode();
        if(node != null) {
            root.children.push_back(node);
        }
    }
    
    return root;
}

func (md : &mut MdParser) parseBlockNode() : *mut MdNode {
    const token = get_token(md);
    const t = token.type;
    
    if(isTextToken(t as int) && isWhitespaceOnlyText(token.value)) {
        increment(md);
        return null; // Skip empty/whitespace lines
    }
    
    if(isNewlineToken(t as int)) {
        increment(md);
        return null;
    }
    
    if(isHashToken(t as int)) return md.parseHeader();
    if(isFencedCodeStartToken(t as int)) return md.parseFencedCodeBlock();
    if(isGreaterThanToken(t as int)) return md.parseBlockquote();
    
    if(isDashToken(t as int) || isPlusToken(t as int) || isStarToken(t as int)) {
        if(isListStart(md)) return md.parseList(false, 1);
        if(isStarToken(t as int) || isDashToken(t as int)) {
             if(md.isHorizontalRule()) return md.parseHorizontalRule();
        }
    }
    
    if(isUnderscoreToken(t as int)) {
        if(md.isHorizontalRule()) return md.parseHorizontalRule();
    }
    
    if(isOrderedListStart(md)) return md.parseOrderedList();
    if(isPipeToken(t as int)) return md.parseTable();
    
    if(isLBracketToken(t as int)) {
        if(isCaretToken(peek_token(md).type as int)) return md.parseFootnoteDef();
        // Check for reference def
        var i = 1u;
        while(true) {
            const tok = peek_token_at(md, i);
            if(isRBracketToken(tok.type as int)) {
                if(isColonToken(peek_token_at(md, i + 1u).type as int)) {
                    return md.parseReferenceDefinition();
                }
                break;
            }
            if(isLineEnd(tok.type as int)) break;
            i++;
        }
    }
    
    if(isStarToken(t as int) && isLBracketToken(peek_token(md).type as int)) {
        // Abbreviation check more robustly?
         var i = 2u;
        while(true) {
            const tok = peek_token_at(md, i);
            if(isRBracketToken(tok.type as int)) {
                const next2 = peek_token_at(md, i + 1u);
                if(isColonToken(next2.type as int)) return md.parseAbbreviation();
                break;
            }
            if(isLineEnd(tok.type as int)) break;
            i++;
        }
    }
    
    if(isColonToken(t as int)) {
        if(isColonToken(peek_token(md).type as int) && isColonToken(peek_token_at(md, 2).type as int)) {
            return md.parseCustomContainer();
        }
        return md.parseDefinitionList();
    }
    
    return md.parseParagraph();
}

func (md : &mut MdParser) isHorizontalRule() : bool {
    const first_type = get_token(md).type;
    var count = 0;
    var i = 0u;
    while(true) {
        const tok = peek_token_at(md, i);
        const t = tok.type;
        if(t == first_type) {
            count++;
            i++;
        } else if(isTextToken(t as int) && isWhitespaceOnlyText(tok.value)) {
            i++;
        } else if(isNewlineToken(t as int) || isBlockEnd(t as int)) {
            break;
        } else {
            return false;
        }
    }
    return count >= 3;
}

func (md : &mut MdParser) parseHorizontalRule() : *mut MdNode {
    while(!isLineEnd(get_token(md).type as int)) {
        increment(md);
    }
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    
    const arena = md.arena;
    var hr = arena.allocate<MdHorizontalRule>();
    new (hr) MdHorizontalRule { base : MdNode { kind : MdNodeKind.Hr } };
    return hr as *mut MdNode;
}

func (md : &mut MdParser) parseHeader() : *mut MdNode {
    var level = 0;
    while(isHashToken(get_token(md).type as int)) {
        level++;
        increment(md);
    }
    if(level > 6) level = 6;
    
    const arena = md.arena;
    var header = arena.allocate<MdHeader>();
    new (header) MdHeader {
        base : MdNode { kind : MdNodeKind.Header },
        level : level,
        children : std::vector<*mut MdNode>()
    }
    
    while(!isLineEnd(get_token(md).type as int)) {
        var node = md.parseInlineNode();
        if(node != null) header.children.push_back(node);
    }
    
    // Trim leading spaces in first text node
    if(header.children.size() > 0) {
        var first = header.children.get(0);
        if(first.kind == MdNodeKind.Text) {
            var txtNode = first as *mut MdText;
            if(txtNode.value.size() > 0 && txtNode.value.data()[0] == ' ') {
                var spaces = 0u;
                while(spaces < txtNode.value.size() && txtNode.value.data()[spaces] == ' ') {
                    spaces++;
                }
                if(spaces < txtNode.value.size()) {
                     txtNode.value = std::string_view(txtNode.value.data() + spaces, txtNode.value.size() - spaces);
                } else {
                    txtNode.value = std::string_view("");
                }
            }
        }
    }
    
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    return header as *mut MdNode;
}

func (md : &mut MdParser) parseFencedCodeBlock() : *mut MdNode {
    const lang = get_token(md).value;
    increment(md);
    
    var code = std::string();
    while(!isFencedCodeEndToken(get_token(md).type as int) && !isBlockEnd(get_token(md).type as int)) {
        if(isCodeContentToken(get_token(md).type as int)) {
            code.append_view(get_token(md).value);
            code.append('\n');
        }
        increment(md);
    }
    if(isFencedCodeEndToken(get_token(md).type as int)) increment(md);
    
    const arena = md.arena;
    
    // We need to store code string logic. Arena doesn't manage string memory automatically unless we copy to arena.
    // std::string owns memory. MdCodeBlock stores std::string_view.
    // WE MUST ALLOCATE STRING DATA ON ARENA or keep ownership.
    // For now, I will assume we need to copy to arena to be safe if MdCodeBlock only holds string_view and std::string is temporary.
    // Since `code` is local std::string, its buffer is freed at end of func.
    // We need to copy `code` to arena.
    // I need an arena allocator for strings/bytes? 
    // I can use arena.allocate_size(size) and memcpy.
    
    // But wait, `Token` values are views into source. `code` here is built up.
    // I'll implement a helper to dup string to arena?
    // Or just use std::string in AST?
    // AST uses `std::string_view`.
    // I will implement string dup on Arena.
    
    const code_len = code.size();
    var code_ptr = arena.allocate_size(code_len + 1, 1) as *mut char; // +1 for null term if nice, but view doesn't need it.
    memcpy(code_ptr, code.data(), code_len);
    // Assuming memcpy exists or I need loop
    // std::memcpy is available?
    // `lang/libs/std` usually exposes libc functions.
    // I'll assume unsafe { ... } copy loop if needed, or use std::memcpy if available.
    // Checking main.ch `using std::Result;`.
    // I'll use a loop to be safe if I'm not sure about memcpy availability in this context.
    
    // Actually, let's check if we can use a simpler approach.
    // For now, I'll allow myself to use `memcpy` assuming it's imported or I can use loops.
    // I will use loop for portability in this context.
    
    unsafe {
        var src = code.data();
        var i = 0u;
        while(i < code_len) {
            *(code_ptr + i) = *(src + i);
            i++;
        }
        *(code_ptr + code_len) = '\0';
    }
    
    var cb = arena.allocate<MdCodeBlock>();
    new (cb) MdCodeBlock {
        base : MdNode { kind : MdNodeKind.CodeBlock },
        language : lang, // language is view into source, so it's fine
        code : std::string_view(code_ptr, code_len)
    }
    return cb as *mut MdNode;
}

func (md : &mut MdParser) parseBlockquote() : *mut MdNode {
    var depth = 0;
    while(isGreaterThanToken(get_token(md).type as int)) {
        depth++;
        increment(md);
        if(isTextToken(get_token(md).type as int) && get_token(md).value.size() > 0 && get_token(md).value.data()[0] == ' ') {
            if(get_token(md).value.size() == 1) increment(md);
        }
    }
    
    const arena = md.arena;
    var root_bq = arena.allocate<MdBlockquote>();
    new (root_bq) MdBlockquote { base : MdNode { kind : MdNodeKind.Blockquote }, children : std::vector<*mut MdNode>() };
    
    var current_container = root_bq;
    var d = 1;
    while(d < depth) {
        var nested = arena.allocate<MdBlockquote>();
        new (nested) MdBlockquote { base : MdNode { kind : MdNodeKind.Blockquote }, children : std::vector<*mut MdNode>() };
        current_container.children.push_back(nested as *mut MdNode);
        current_container = nested;
        d++;
    }
    
    while(!isLineEnd(get_token(md).type as int)) {
        var node = md.parseInlineNode();
        if(node != null) current_container.children.push_back(node);
    }
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    return root_bq as *mut MdNode;
}

func (md : &mut MdParser) parseList(ordered : bool, start : int) : *mut MdNode {
    const arena = md.arena;
    var list = arena.allocate<MdList>();
    new (list) MdList {
        base : MdNode { kind : MdNodeKind.List },
        ordered : ordered,
        start : start,
        children : std::vector<*mut MdNode>()
    }
    
    // Track the indentation level of this list's items
    var list_indent = -1;  // Will be set on first item
    
    while(true) {
        // Skip newlines
        while(isNewlineToken(get_token(md).type as int)) { increment(md) };
        
        // Measure indentation (whitespace before list marker)
        var current_indent = 0;
        while(isTextToken(get_token(md).type as int) && isWhitespaceOnlyText(get_token(md).value)) {
            current_indent = current_indent + get_token(md).value.size() as int;
            increment(md);
        }
        
        // Check for list marker
        if(!(isDashToken(get_token(md).type as int) || isPlusToken(get_token(md).type as int) || isStarToken(get_token(md).type as int))) {
            break;
        }
        
        // Set the expected indent on first item
        if(list_indent < 0) {
            list_indent = current_indent;
        }
        
        // If indentation doesn't match, we're done with this list
        // Items with less indentation belong to parent list
        // Items with more indentation would have been handled as nested
        if(current_indent != list_indent) {
            // Push back the tokens we consumed (can't easily do this)
            // Instead, just break - let caller handle it
            break;
        }
    
        increment(md);  // consume list marker
        
        // Skip leading space of item
        if(isTextToken(get_token(md).type as int) && get_token(md).value.size() > 0 && get_token(md).value.data()[0] == ' ') {
            // Will be handled in inline parsing
        }
        
        var item = arena.allocate<MdListItem>();
        new (item) MdListItem { base : MdNode { kind : MdNodeKind.ListItem }, children : std::vector<*mut MdNode>() };
        
        var cb = md.tryParseTaskCheckbox();
        if(cb != null) item.children.push_back(cb);
        
        // Parse inline content of this item
        while(!isLineEnd(get_token(md).type as int)) {
            var node = md.parseInlineNode();
            if(node != null) item.children.push_back(node);
        }
        if(isNewlineToken(get_token(md).type as int)) increment(md);
        
        // Check for nested list (deeper indentation)
        // Peek at next line's indentation
        var peek_indent = 0;
        var saved_pos = md.pos;
        while(isNewlineToken(get_token(md).type as int)) { increment(md) };
        while(isTextToken(get_token(md).type as int) && isWhitespaceOnlyText(get_token(md).value)) {
            peek_indent = peek_indent + get_token(md).value.size() as int;
            increment(md);
        }
        
        // If next line is a list item with MORE indentation, it's a nested list
        if(peek_indent > list_indent && (isDashToken(get_token(md).type as int) || isPlusToken(get_token(md).type as int) || isStarToken(get_token(md).type as int))) {
            // Restore position to let nested parseList measure its own indentation
            md.pos = saved_pos;
            // Now parse nested list - it will handle its own newline/whitespace skipping
            var nested = md.parseList(false, 1);
            if(nested != null) item.children.push_back(nested);
        } else {
            // Restore position for next iteration
            md.pos = saved_pos;
        }
        
        list.children.push_back(item as *mut MdNode);
    }
    return list as *mut MdNode;
}

func (md : &mut MdParser) parseOrderedList() : *mut MdNode {
    var start_num = 1;
    if(isNumberToken(get_token(md).type as int)) {
        const num_str = get_token(md).value;
        start_num = 0;
        var i = 0u;
        while(i < num_str.size()) {
            start_num = start_num * 10 + (num_str.data()[i] - '0') as int;
            i++;
        }
    }
    
    const arena = md.arena;
    var list = arena.allocate<MdList>();
    new (list) MdList {
        base : MdNode { kind : MdNodeKind.List },
        ordered : true,
        start : start_num,
        children : std::vector<*mut MdNode>()
    }
    
    while(true) {
        while(isNewlineToken(get_token(md).type as int)) { increment(md) };
        
        if(!(isNumberToken(get_token(md).type as int))) break;
        const next = peek_token(md);
        if(!isDotToken(next.type as int)) break;
        
        increment(md); // number
        increment(md); // dot
        
        var item = arena.allocate<MdListItem>();
        new (item) MdListItem { base : MdNode { kind : MdNodeKind.ListItem }, children : std::vector<*mut MdNode>() };
        
        var cb = md.tryParseTaskCheckbox();
        if(cb != null) item.children.push_back(cb);
        
        while(!isLineEnd(get_token(md).type as int)) {
            var node = md.parseInlineNode();
            if(node != null) item.children.push_back(node);
        }
        if(isNewlineToken(get_token(md).type as int)) increment(md);
        
        while(true) {
            const val = get_token(md).value;
            if(isTextToken(get_token(md).type as int) && val.size() >= 2 && val.data()[0] == ' ' && val.data()[1] == ' ') {
                if(isWhitespaceOnlyText(val)) {
                    increment(md);
                    skipWhitespace(md);
                    if(isBlockStart(md)) {
                         var sub = md.parseBlockNode();
                         if(sub != null) item.children.push_back(sub);
                         continue;
                    }
                    continue;
                }
                if(isListStart(md) || isOrderedListStart(md) || isBlockStart(md)) {
                     var sub = md.parseBlockNode();
                     if(sub != null) item.children.push_back(sub);
                     continue;
                }
                while(!isLineEnd(get_token(md).type as int)) {
                    var node = md.parseInlineNode();
                    if(node != null) item.children.push_back(node);
                }
                if(isNewlineToken(get_token(md).type as int)) increment(md);
                continue;
            }
            break;
        }
        
        list.children.push_back(item as *mut MdNode);
        
        while(isTextToken(get_token(md).type as int) && isWhitespaceOnlyText(get_token(md).value)) {
            increment(md);
        }
        if(isNewlineToken(get_token(md).type as int)) increment(md);
    }
    return list as *mut MdNode;
}

func (md : &mut MdParser) parseTable() : *mut MdNode {
    const arena = md.arena;
    var table = arena.allocate<MdTable>();
    new (table) MdTable {
         base : MdNode { kind : MdNodeKind.Table },
         alignments : std::vector<MdTableAlign>(),
         children : std::vector<*mut MdNode>()
    };
    
    var header = md.parseTableRow(true);
    if(header != null) table.children.push_back(header);
    
    if(isPipeToken(get_token(md).type as int)) {
        md.parseTableAlignmentRow(table);
    }
    
    while(isPipeToken(get_token(md).type as int)) {
        var row = md.parseTableRow(false);
        if(row != null) table.children.push_back(row);
    }
    return table as *mut MdNode;
}

func (md : &mut MdParser) parseTableRow(is_header : bool) : *mut MdNode {
    const arena = md.arena;
    var row = arena.allocate<MdTableRow>();
    new (row) MdTableRow { base : MdNode { kind : MdNodeKind.TableRow }, is_header : is_header, children : std::vector<*mut MdNode>() };
    
    if(isPipeToken(get_token(md).type as int)) increment(md);
    
    while(!isLineEnd(get_token(md).type as int)) {
        var cell = arena.allocate<MdTableCell>();
        new (cell) MdTableCell { base : MdNode { kind : MdNodeKind.TableCell }, children : std::vector<*mut MdNode>() };
        
        while(!isPipeToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
            var node = md.parseInlineNode();
            if(node != null) cell.children.push_back(node);
        }
        row.children.push_back(cell as *mut MdNode);
        
        if(isPipeToken(get_token(md).type as int)) {
            increment(md);
            skipWhitespace(md);
            if(isLineEnd(get_token(md).type as int)) break;
        }
    }
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    return row as *mut MdNode;
}

func (md : &mut MdParser) parseTableAlignmentRow(table : *mut MdTable) {
    if(isPipeToken(get_token(md).type as int)) increment(md);
    while(!isLineEnd(get_token(md).type as int)) {
        var align = MdTableAlign.None;
        var has_left = false;
        var has_right = false;
        if(isColonToken(get_token(md).type as int)) { has_left = true; increment(md); }
        while(isDashToken(get_token(md).type as int)) { increment(md); }
        if(isColonToken(get_token(md).type as int)) { has_right = true; increment(md); }
        
        if(has_left && has_right) align = MdTableAlign.Center;
        else if(has_left) align = MdTableAlign.Left;
        else if(has_right) align = MdTableAlign.Right;
        
        table.alignments.push_back(align);
        
        if(isTextToken(get_token(md).type as int) && isWhitespaceOnlyText(get_token(md).value)) increment(md);
        if(isPipeToken(get_token(md).type as int)) increment(md);
    }
    if(isNewlineToken(get_token(md).type as int)) increment(md);
}

func (md : &mut MdParser) parseParagraph() : *mut MdNode {
    const arena = md.arena;
    var para = arena.allocate<MdParagraph>();
    new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
    
    while(!isBlockEnd(get_token(md).type as int)) {
        if(isNewlineToken(get_token(md).type as int)) {
            increment(md);
            if(isNewlineToken(get_token(md).type as int) || isBlockStart(md)) break;
            continue;
        }
        var node = md.parseInlineNode();
        if(node != null) para.children.push_back(node);
    }
    if(para.children.size() == 0) return null;
    return para as *mut MdNode;
}

func (md : &mut MdParser) parseInlineNode() : *mut MdNode {
    const t = get_token(md).type;
    const arena = md.arena;
    
    if(isTextToken(t as int)) {
        var text = arena.allocate<MdText>();
        new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : get_token(md).value };
        increment(md);
        return text as *mut MdNode;
    }
    // No chemical start token support (Lexer returns Text "$")
    
    if(isStarToken(t as int)) return md.parseBoldOrItalic('*');
    if(isUnderscoreToken(t as int)) return md.parseBoldOrItalic('_');
    
    if(isTildeToken(t as int)) {
        if(isTildeToken(peek_token(md).type as int)) return md.parseStrikethrough();
        return md.parseSubscript();
    }
    if(isCaretToken(t as int)) return md.parseSuperscript();
    if(isEqualToken(t as int)) {
        if(isEqualToken(peek_token(md).type as int)) return md.parseMark();
    }
    if(isPlusToken(t as int)) {
        if(isPlusToken(peek_token(md).type as int)) return md.parseInsert();
    }
    if(isLBracketToken(t as int)) {
        if(isCaretToken(peek_token(md).type as int)) return md.parseFootnote();
        return md.parseLink();
    }
    if(isExclamationToken(t as int)) return md.parseImage();
    if(isBacktickToken(t as int)) return md.parseInlineCode();
    
    var text = arena.allocate<MdText>();
    new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : get_token(md).value };
    increment(md);
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseBoldOrItalic(marker : char) : *mut MdNode {
    const marker_type = if(marker == '*') MdTokenType.Star as int else MdTokenType.Underscore as int;
    var count = 0;
    while(get_token(md).type as int == marker_type) {
        count++;
        increment(md);
    }
    const arena = md.arena;
    
    // Lookahead to ensure we have a closing marker on same line
    var has_closing = false;
    var look = 0u;
    while(true) {
        const tok = peek_token_at(md, look);
        if(isLineEnd(tok.type as int)) break;
        
        // If inside a link, RBracket closes the scope for lookahead
        if(md.in_link && isRBracketToken(tok.type as int)) break;
        
        if(tok.type as int == marker_type) {
             if(count == 1) {
                 has_closing = true;
                 break;
             } else {
                 // For bold, we need 2 markers
                 const next_tok = peek_token_at(md, look + 1u);
                 if(next_tok.type as int == marker_type) {
                      has_closing = true;
                      break;
                 }
                 // If we have just 1 marker but we need 2, we continue looking?
                 // Existing logic treats "**" as end of bold.
                 // A single "*" inside bold is treated as text.
             }
        }
        look++;
    }
    
    if(!has_closing) {
        // Treat as text
        // Optimize for common cases
        if(count == 1) {
             var text = arena.allocate<MdText>();
             new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : if(marker == '*') std::string_view("*") else std::string_view("_") };
             return text as *mut MdNode;
        }
        if(count == 2) {
             var text = arena.allocate<MdText>();
             new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : if(marker == '*') std::string_view("**") else std::string_view("__") };
             return text as *mut MdNode;
        }
        
        // General case
        var ptr = arena.allocate_size((count + 1) as size_t, 1) as *mut char;
        unsafe {
             var j = 0;
             while(j < count) { *(ptr + j) = marker; j++; }
             *(ptr + count) = '\0';
        }
        var text = arena.allocate<MdText>();
        new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : std::string_view(ptr, count as size_t) };
        return text as *mut MdNode;
    }
    
    if(count == 1) {
        var italic = arena.allocate<MdItalic>();
        new (italic) MdItalic { base : MdNode { kind : MdNodeKind.Italic }, children : std::vector<*mut MdNode>() };
        while(get_token(md).type as int != marker_type && !isLineEnd(get_token(md).type as int)) {
            var child = md.parseInlineNode();
            if(child != null) italic.children.push_back(child);
        }
        if(get_token(md).type as int == marker_type) increment(md);
        return italic as *mut MdNode;
    } else {
        var bold = arena.allocate<MdBold>();
        new (bold) MdBold { base : MdNode { kind : MdNodeKind.Bold }, children : std::vector<*mut MdNode>() };
        while(true) {
            if(isLineEnd(get_token(md).type as int)) break;
            if(get_token(md).type as int == marker_type) {
                increment(md);
                if(get_token(md).type as int == marker_type) {
                    increment(md);
                    break;
                }
                 var markerText = arena.allocate<MdText>();
                 new (markerText) MdText { base : MdNode { kind : MdNodeKind.Text }, value : if(marker == '*') std::string_view("*") else std::string_view("_") };
                 bold.children.push_back(markerText as *mut MdNode);
                 continue;
            }
            var child = md.parseInlineNode();
            if(child != null) bold.children.push_back(child);
        }
        return bold as *mut MdNode;
    }
}

func (md : &mut MdParser) parseStrikethrough() : *mut MdNode {
    increment(md); increment(md);
    const arena = md.arena;
    var strike = arena.allocate<MdStrikethrough>();
    new (strike) MdStrikethrough { base : MdNode { kind : MdNodeKind.Strikethrough }, children : std::vector<*mut MdNode>() };
    while(true) {
        if(isLineEnd(get_token(md).type as int)) break;
        if(isTildeToken(get_token(md).type as int) && isTildeToken(peek_token(md).type as int)) {
            increment(md); increment(md); break;
        }
        var child = md.parseInlineNode();
        if(child != null) strike.children.push_back(child);
    }
    return strike as *mut MdNode;
}

func (md : &mut MdParser) parseSubscript() : *mut MdNode {
    increment(md);
    const arena = md.arena;
    var sub = arena.allocate<MdSubscript>();
    new (sub) MdSubscript { base : MdNode { kind : MdNodeKind.Subscript }, children : std::vector<*mut MdNode>() };
    while(!isTildeToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        var child = md.parseInlineNode();
        if(child != null) sub.children.push_back(child);
    }
    if(isTildeToken(get_token(md).type as int)) increment(md);
    return sub as *mut MdNode;
}

func (md : &mut MdParser) parseSuperscript() : *mut MdNode {
    increment(md);
    const arena = md.arena;
    var sup = arena.allocate<MdSuperscript>();
    new (sup) MdSuperscript { base : MdNode { kind : MdNodeKind.Superscript }, children : std::vector<*mut MdNode>() };
    while(!isCaretToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        var child = md.parseInlineNode();
        if(child != null) sup.children.push_back(child);
    }
    if(isCaretToken(get_token(md).type as int)) increment(md);
    return sup as *mut MdNode;
}

func (md : &mut MdParser) parseMark() : *mut MdNode {
    increment(md); increment(md);
    const arena = md.arena;
    var mark = arena.allocate<MdMark>();
    new (mark) MdMark { base : MdNode { kind : MdNodeKind.Mark }, children : std::vector<*mut MdNode>() };
    while(true) {
        if(isLineEnd(get_token(md).type as int)) break;
        if(isEqualToken(get_token(md).type as int) && isEqualToken(peek_token(md).type as int)) {
            increment(md); increment(md); break;
        }
        var child = md.parseInlineNode();
        if(child != null) mark.children.push_back(child);
    }
    return mark as *mut MdNode;
}

func (md : &mut MdParser) parseInsert() : *mut MdNode {
    increment(md); increment(md);
    const arena = md.arena;
    var ins = arena.allocate<MdInsert>();
    new (ins) MdInsert { base : MdNode { kind : MdNodeKind.Insert }, children : std::vector<*mut MdNode>() };
    while(true) {
        if(isLineEnd(get_token(md).type as int)) break;
        if(isPlusToken(get_token(md).type as int) && isPlusToken(peek_token(md).type as int)) {
            increment(md); increment(md); break;
        }
        var child = md.parseInlineNode();
        if(child != null) ins.children.push_back(child);
    }
    return ins as *mut MdNode;
}

func (md : &mut MdParser) parseFootnote() : *mut MdNode {
    increment(md); // [
    increment(md); // ^
    var id = std::string();
    while(!isRBracketToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        id.append_view(get_token(md).value);
        increment(md);
    }
    if(isRBracketToken(get_token(md).type as int)) increment(md);
    
    const arena = md.arena;
    // copy id string to arena
    const len = id.size();
    var ptr = arena.allocate_size(len + 1, 1) as *mut char;
    unsafe {
        var src = id.data();
        var i = 0u;
        while(i < len) { *(ptr + i) = *(src + i); i++; }
        *(ptr + len) = '\0';
    }
    
    var fn = arena.allocate<MdFootnote>();
    new (fn) MdFootnote { base : MdNode { kind : MdNodeKind.Footnote }, id : std::string_view(ptr, len) };
    return fn as *mut MdNode;
}

func (md : &mut MdParser) parseFootnoteDef() : *mut MdNode {
    increment(md); // [
    increment(md); // ^
    var id = std::string();
    while(!isRBracketToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        id.append_view(get_token(md).value);
        increment(md);
    }
    if(isRBracketToken(get_token(md).type as int)) increment(md);
    if(isColonToken(get_token(md).type as int)) increment(md);
    
    const arena = md.arena;
    const len = id.size();
    var ptr = arena.allocate_size(len + 1, 1) as *mut char;
    unsafe {
        var src = id.data();
        var i = 0u;
        while(i < len) { *(ptr + i) = *(src + i); i++; }
        *(ptr + len) = '\0';
    }
    
    var fn = arena.allocate<MdFootnoteDef>();
    new (fn) MdFootnoteDef {
        base : MdNode { kind : MdNodeKind.FootnoteDef },
        id : std::string_view(ptr, len),
        children : std::vector<*mut MdNode>()
    }
    while(!isNewlineToken(get_token(md).type as int) && !isBlockEnd(get_token(md).type as int)) {
        var node = md.parseInlineNode();
        if(node != null) fn.children.push_back(node);
    }
    return fn as *mut MdNode;
}

func (md : &mut MdParser) parseAbbreviation() : *mut MdNode {
    increment(md); // *
    increment(md); // [
    var id = std::string();
    while(!isRBracketToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        id.append_view(get_token(md).value);
        increment(md);
    }
    if(isRBracketToken(get_token(md).type as int)) increment(md);
    if(isColonToken(get_token(md).type as int)) increment(md);
    
    if(isTextToken(get_token(md).type as int) && isWhitespaceOnlyText(get_token(md).value)) increment(md);
    
    var title = std::string();
    while(!isLineEnd(get_token(md).type as int)) {
        title.append_view(get_token(md).value);
        increment(md);
    }
    
    const arena = md.arena;
    // copy id and title
    const id_len = id.size();
    var id_ptr = arena.allocate_size(id_len + 1, 1) as *mut char;
    if(true) {
        unsafe { var s = id.data(); var i = 0u; while(i < id_len) { *(id_ptr + i) = *(s + i); i++; } *(id_ptr + id_len) = '\0'; }
    }
    
    const ti_len = title.size();
    var ti_ptr = arena.allocate_size(ti_len + 1, 1) as *mut char;
    if(true) {
        unsafe { var s = title.data(); var i = 0u; while(i < ti_len) { *(ti_ptr + i) = *(s + i); i++; } *(ti_ptr + ti_len) = '\0'; }
    }

    var abb = arena.allocate<MdAbbreviation>();
    new (abb) MdAbbreviation {
        base : MdNode { kind : MdNodeKind.Abbreviation },
        id : std::string_view(id_ptr, id_len),
        title : std::string_view(ti_ptr, ti_len)
    };
    return abb as *mut MdNode;
}

func (md : &mut MdParser) parseCustomContainer() : *mut MdNode {
    increment(md); increment(md); increment(md);
    const arena = md.arena;
    
    var type_view = std::string_view("");
    if(isTextToken(get_token(md).type as int)) {
         type_view = trim_view(get_token(md).value);
         increment(md);
    }
    while(!isNewlineToken(get_token(md).type as int) && !isBlockEnd(get_token(md).type as int)) { increment(md) };
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    
    var container = arena.allocate<MdCustomContainer>();
    new (container) MdCustomContainer {
        base : MdNode { kind : MdNodeKind.CustomContainer },
        type : type_view,
        children : std::vector<*mut MdNode>()
    }
    while(!isBlockEnd(get_token(md).type as int)) {
        const t = get_token(md).type;
        if(isColonToken(t as int)) {
            if(isColonToken(peek_token(md).type as int) && isColonToken(peek_token_at(md, 2).type as int)) {
                increment(md); increment(md); increment(md);
                break;
            }
        }
        var node = md.parseBlockNode();
        if(node != null) container.children.push_back(node);
    }
    return container as *mut MdNode;
}

func (md : &mut MdParser) parseDefinitionList() : *mut MdNode {
    const arena = md.arena;
    var dl = arena.allocate<MdDefinitionList>();
    new (dl) MdDefinitionList { base : MdNode { kind : MdNodeKind.DefinitionList }, children : std::vector<*mut MdNode>() };
    
    // Check preceding paragraph
    if(md.root.children.size() > 0) {
        var last = md.root.children.get(md.root.children.size() - 1);
        if(last.kind == MdNodeKind.Paragraph) {
            var para = last as *mut MdParagraph;
            var term = arena.allocate<MdDefinitionTerm>();
            // Moving children
            var children = std::vector<*mut MdNode>();
             var i = 0u;
             while(i < para.children.size()) { children.push_back(para.children.get(i)); i++; }
             
            new (term) MdDefinitionTerm {
                base : MdNode { kind : MdNodeKind.DefinitionTerm },
                children : children
            }
            dl.children.push_back(term as *mut MdNode);
            md.root.children.pop_back(); 
        }
    }
    
    while(isColonToken(get_token(md).type as int) || (isTextToken(get_token(md).type as int) && !isBlockStart(md))) {
        if(isColonToken(get_token(md).type as int)) {
            increment(md); skipWhitespace(md);
            var data = arena.allocate<MdDefinitionData>();
            new (data) MdDefinitionData { base : MdNode { kind : MdNodeKind.DefinitionData }, children : std::vector<*mut MdNode>() };
            while(!isLineEnd(get_token(md).type as int)) {
                var node = md.parseInlineNode();
                if(node != null) data.children.push_back(node);
            }
            dl.children.push_back(data as *mut MdNode);
            if(isNewlineToken(get_token(md).type as int)) increment(md);
        } else {
             var term = arena.allocate<MdDefinitionTerm>();
             new (term) MdDefinitionTerm { base : MdNode { kind : MdNodeKind.DefinitionTerm }, children : std::vector<*mut MdNode>() };
             while(!isLineEnd(get_token(md).type as int)) {
                 var node = md.parseInlineNode();
                 if(node != null) term.children.push_back(node);
             }
             dl.children.push_back(term as *mut MdNode);
             if(isNewlineToken(get_token(md).type as int)) increment(md);
        }
        skipWhitespace(md);
        if(isBlockEnd(get_token(md).type as int)) break;
    }
    return dl as *mut MdNode;
}

func (md : &mut MdParser) parseUrl() : std::string_view {
    var url_res = std::string();
    const arena = md.arena;
    while(true) {
        const t = get_token(md).type;
        if(isTextToken(t as int) || isColonToken(t as int) || isDotToken(t as int) || isDashToken(t as int) || isPlusToken(t as int) || isUnderscoreToken(t as int) || isNumberToken(t as int) || isHashToken(t as int) || isPipeToken(t as int)) {
            url_res.append_view(get_token(md).value);
            increment(md);
            continue;
        }
        if(t == MdTokenType.RParen || t == MdTokenType.RBracket || isLineEnd(t as int) || (isTextToken(t as int) && get_token(md).value.data()[0] == ' ')) break;
        if(isEndMdToken(t as int) || t == MdTokenType.RBrace) break;
        
        url_res.append_view(get_token(md).value);
        increment(md);
    }
    
    // Dup to arena
    const len = url_res.size();
    if(len == 0) return std::string_view("");
    var ptr = arena.allocate_size(len + 1, 1) as *mut char;
    unsafe { var s = url_res.data(); var i = 0u; while(i < len) { *(ptr + i) = *(s + i); i++; } *(ptr + len) = '\0'; }
    return std::string_view(ptr, len);
}

func (md : &mut MdParser) parseLink() : *mut MdNode {
    increment(md); // [
    
    // Save state
    const prev_in_link = md.in_link;
    md.in_link = true;
    
    var text_children = std::vector<*mut MdNode>();
    while(get_token(md).type != MdTokenType.RBracket && !isLineEnd(get_token(md).type as int)) {
        var child = md.parseInlineNode();
        if(child != null) text_children.push_back(child);
    }
    
    // Restore state
    md.in_link = prev_in_link;
    
    const arena = md.arena;
    if(get_token(md).type == MdTokenType.RBracket) {
        increment(md);
        if(get_token(md).type == MdTokenType.LParen) {
             increment(md);
             var url = md.parseUrl();
             var title = std::string_view("");
             
             if(isTextToken(get_token(md).type as int)) {
                 const txt = get_token(md).value;
                 if(txt.size() > 0 && txt.data()[0] == ' ') {
                     increment(md);
                     if(isTextToken(get_token(md).type as int)) {
                         const ttxt = get_token(md).value;
                         if(ttxt.size() >= 2 && ttxt.data()[0] == '"') {
                              if(ttxt.data()[ttxt.size()-1] == '"') {
                                  title = std::string_view(ttxt.data() + 1, ttxt.size() - 2);
                              } else {
                                  title = ttxt;
                              }
                              increment(md);
                         }
                     }
                 }
             }
             if(get_token(md).type == MdTokenType.RParen) {
                 increment(md);
                 var link = arena.allocate<MdLink>();
                 new (link) MdLink { base : MdNode { kind : MdNodeKind.Link }, url : url, title : title, children : text_children };
                 return link as *mut MdNode;
             }
        } else if(get_token(md).type == MdTokenType.LBracket) {
             // Reference link support omitted for brevity/complexity in first pass, or assuming simple link
        }
    }
    // Fallback: [ text
    var text = arena.allocate<MdText>();
    new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : std::string_view("[") };
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseImage() : *mut MdNode {
    increment(md); // !
    const arena = md.arena;
    if(isLBracketToken(get_token(md).type as int)) {
        increment(md); // [
        var alt = std::string_view("");
        if(isTextToken(get_token(md).type as int)) {
            alt = get_token(md).value;
            increment(md);
        }
        if(get_token(md).type == MdTokenType.RBracket) increment(md);
        if(get_token(md).type == MdTokenType.LParen) {
            increment(md);
            var url = md.parseUrl();
            if(get_token(md).type == MdTokenType.RParen) increment(md);
            
            var img = arena.allocate<MdImage>();
            new (img) MdImage { base : MdNode { kind : MdNodeKind.Image }, url : url, alt : alt, title : std::string_view("") };
            return img as *mut MdNode;
        }
    }
    var text = arena.allocate<MdText>();
    new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : std::string_view("!") };
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseInlineCode() : *mut MdNode {
    const delim = get_token(md).value;
    const delim_size = delim.size();
    increment(md);
    
    var content = std::string();
    while(!isLineEnd(get_token(md).type as int)) {
        const t = get_token(md);
        if(t.type == MdTokenType.Backtick && t.value.size() == delim_size) {
            increment(md);
            const arena = md.arena;
            const len = content.size();
            var ptr = arena.allocate_size(len + 1, 1) as *mut char;
            unsafe { var s = content.data(); var i = 0u; while(i < len) { *(ptr + i) = *(s + i); i++; } *(ptr + len) = '\0'; }
            
            var code = arena.allocate<MdInlineCode>();
            new (code) MdInlineCode { base : MdNode { kind : MdNodeKind.InlineCode }, value : std::string_view(ptr, len) };
            return code as *mut MdNode;
        }
        content.append_view(t.value);
        increment(md);
    }
    // Failed to find close, treat as text
    const arena = md.arena;
    var text = arena.allocate<MdText>();
    new (text) MdText { base : MdNode { kind : MdNodeKind.Text }, value : delim };
    return text as *mut MdNode;
}

func (md : &mut MdParser) parseReferenceDefinition() : *mut MdNode {
    increment(md); // [
    var id = std::string();
    while(!isRBracketToken(get_token(md).type as int) && !isLineEnd(get_token(md).type as int)) {
        id.append_view(get_token(md).value);
        increment(md);
    }
    if(isRBracketToken(get_token(md).type as int)) increment(md);
    if(isColonToken(get_token(md).type as int)) increment(md);
    skipWhitespace(md);
    var url = md.parseUrl();
    
    skipWhitespace(md);
    var title = std::string_view("");
    if(isTextToken(get_token(md).type as int)) {
        const ttxt = get_token(md).value;
        if(ttxt.size() >= 2 && ttxt.data()[0] == '"' && ttxt.data()[ttxt.size()-1] == '"') {
            title = std::string_view(ttxt.data() + 1, ttxt.size() - 2);
            // We need to copy title to arena because logic expects views to live
            const arena = md.arena;
            const len = title.size();
            var ptr = arena.allocate_size(len + 1, 1) as *mut char;
            unsafe { var s = title.data(); var i = 0u; while(i < len) { *(ptr + i) = *(s + i); i++; } *(ptr + len) = '\0'; }
            title = std::string_view(ptr, len);
            increment(md);
        }
    }
    
    const arena = md.arena;
    const len = id.size();
    var ptr = arena.allocate_size(len + 1, 1) as *mut char;
    unsafe { var s = id.data(); var i = 0u; while(i < len) { *(ptr + i) = *(s + i); i++; } *(ptr + len) = '\0'; }
    var id_view = std::string_view(ptr, len);
    
    md.root.reference_defs.insert(std::string(id_view), MdReference { url : url, title : title });
    if(isNewlineToken(get_token(md).type as int)) increment(md);
    return null; // Not a visible node
}

} // namespace md
