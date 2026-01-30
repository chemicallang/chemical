public namespace md {

public struct TokenParser {
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

    func peek(&self) : MdToken {
        return self.peek_at(1)
    }

    func peek_ahead(&self, off : size_t) : MdToken {
        return self.peek_at(off)
    }

    func bump(&mut self) { if(self.idx < self.tokens.size()) { self.idx += 1; } }
}

public func is_end(t : int) : bool { return t == MdTokenType.EndOfFile as int; }
public func is_text(t : int) : bool { return t == MdTokenType.Text as int; }
public func is_nl(t : int) : bool { return t == MdTokenType.Newline as int; }
public func is_line_end(t : int) : bool { return is_nl(t) || is_end(t); }

public func is_ws_only(v : std::string_view) : bool {
    var i = 0u;
    while(i < v.size()) {
        const c = v.data()[i];
        if(c != ' ' && c != '\t' && c != '\r') return false;
        i++;
    }
    return true;
}

public func skip_blank_lines(p : &mut TokenParser) {
    while(true) {
        const t = p.get();
        if(is_text(t.type) && is_ws_only(t.value)) { p.bump(); continue; }
        if(is_nl(t.type)) { p.bump(); continue; }
        break;
    }
}

public struct MdParser {
    var arena : *mut Arena

    func make_text(&mut self, v : std::string_view) : *mut MdNode {
        var n = self.arena.allocate<MdText>();
        new (n) MdText { base : MdNode { kind : MdNodeKind.Text }, value : v };
        return n as *mut MdNode;
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

        var n = parse_block(mdp, p)
        if(n != null) root.children.push(n)
        
        printf("parsing block\n");
    }

    return root;
}

}
