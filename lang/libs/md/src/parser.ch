public namespace md {

struct TokenParser {
    var tokens : *mut std::vector<MdToken>
    var idx : size_t

    func init(&mut self, tokens : *mut std::vector<MdToken>) {
        self.tokens = tokens
        self.idx = 0
    }

    func get(&self) : MdToken {
        if(self.idx >= self.tokens.size()) {
            return MdToken { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : 0 }
        }
        return self.tokens.get(self.idx)
    }

    func peek_at(&self, off : size_t) : MdToken {
        const i = self.idx + off
        if(i >= self.tokens.size()) {
            return MdToken { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : 0 }
        }
        return self.tokens.get(i)
    }

    func bump(&mut self) { if(self.idx < self.tokens.size()) { self.idx += 1; } }
}

func is_end(t : int) : bool { return t == MdTokenType.EndOfFile as int; }
func is_text(t : int) : bool { return t == MdTokenType.Text as int; }
func is_nl(t : int) : bool { return t == MdTokenType.Newline as int; }

func is_ws_only(v : std::string_view) : bool {
    var i = 0u;
    while(i < v.size()) {
        const c = v.data()[i];
        if(c != ' ' && c != '\t' && c != '\r') return false;
        i++;
    }
    return true;
}

func skip_blank_lines(p : &mut TokenParser) {
    while(true) {
        const t = p.get();
        if(is_text(t.type) && is_ws_only(t.value)) { p.bump(); continue; }
        if(is_nl(t.type)) { p.bump(); continue; }
        break;
    }
}

struct MdParser {
    var arena : *mut Arena

    func make_text(&mut self, v : std::string_view) : *mut MdNode {
        var n = self.arena.allocate<MdText>();
        new (n) MdText { base : MdNode { kind : MdNodeKind.Text }, value : v };
        return n as *mut MdNode;
    }

    func parse_inline_text_until_line_end(&mut self, p : &mut TokenParser, out_children : &mut std::vector<*mut MdNode>) {
        while(true) {
            const t = p.get();
            if(is_end(t.type) || is_nl(t.type)) break;

            // Minimal inline support: treat everything as text for now.
            if(is_text(t.type)) {
                out_children.push(self.make_text(t.value));
                p.bump();
                continue;
            }

            // fall back: represent punctuation tokens as text
            out_children.push(self.make_text(t.value));
            p.bump();
        }
    }

    func parse_header(&mut self, p : &mut TokenParser) : *mut MdNode {
        // count consecutive #
        var level = 0;
        while(p.get().type == MdTokenType.Hash as int) { level += 1; p.bump(); }
        if(level <= 0) level = 1;

        // optional leading space
        if(is_text(p.get().type) && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
            p.bump();
        }

        var h = self.arena.allocate<MdHeader>();
        new (h) MdHeader { base : MdNode { kind : MdNodeKind.Header }, level : level, children : std::vector<*mut MdNode>() };
        self.parse_inline_text_until_line_end(p, h.children);
        if(is_nl(p.get().type)) p.bump();
        return h as *mut MdNode;
    }

    func parse_fenced_code_block(&mut self, p : &mut TokenParser) : *mut MdNode {
        // current token is FencedCodeStart, value holds language
        const lang = p.get().value;
        p.bump();

        // Allocate the code string in the arena so it lives as long as the AST.
        var code_ptr = self.arena.allocate<std::string>();
        new (code_ptr) std::string();
        while(true) {
            const t = p.get();
            if(t.type == MdTokenType.FencedCodeEnd as int) { p.bump(); break; }
            if(t.type == MdTokenType.CodeContent as int) {
                code_ptr.append_view(t.value);
                code_ptr.append('\n');
                p.bump();
                continue;
            }
            if(is_end(t.type)) break;
            // ignore unexpected tokens
            p.bump();
        }

        var cb = self.arena.allocate<MdCodeBlock>();
        new (cb) MdCodeBlock { base : MdNode { kind : MdNodeKind.CodeBlock }, language : lang, code : code_ptr.to_view() };
        return cb as *mut MdNode;
    }

    func parse_paragraph(&mut self, p : &mut TokenParser) : *mut MdNode {
        var para = self.arena.allocate<MdParagraph>();
        new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
        self.parse_inline_text_until_line_end(p, para.children);
        if(is_nl(p.get().type)) p.bump();
        return para as *mut MdNode;
    }

    func parse_block(&mut self, p : &mut TokenParser) : *mut MdNode {
        const t = p.get();
        if(t.type == MdTokenType.Hash as int) return self.parse_header(p);
        if(t.type == MdTokenType.FencedCodeStart as int) return self.parse_fenced_code_block(p);
        return self.parse_paragraph(p);
    }
}

public func parse(tokens : *mut std::vector<MdToken>, arena : *mut Arena) : *mut MdRoot {
    var p : TokenParser; p.init(tokens)
    var mdp = MdParser { arena : arena }

    var root = arena.allocate<MdRoot>();
    new (root) MdRoot { base : MdNode { kind : MdNodeKind.Root }, children : std::vector<*mut MdNode>() };

    while(!is_end(p.get().type)) {
        skip_blank_lines(p)
        if(is_end(p.get().type)) break
        var n = mdp.parse_block(p)
        if(n != null) root.children.push(n)
    }

    return root;
}

}
