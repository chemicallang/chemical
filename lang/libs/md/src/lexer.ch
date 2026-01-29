public namespace md {

public struct Lexer {
    var text : std::string_view
    var ptr : *char
    var end : *char
    var pos : size_t

    var in_fenced_code : bool
    var fence_count : int

    @make
    func make() {
        text = std::string_view("");
        ptr = "";
        end = ptr + 1;
        pos = 0;
        in_fenced_code = false
        fence_count = 0
    }

    @make
    func make2(text_view : std::string_view) {
        text = text_view
        ptr = text.data()
        end = text.data() + text.size() as isize
        pos = 0
        in_fenced_code = false
        fence_count = 0
    }

    func peek(&self) : char {
        if(self.ptr >= self.end) return '\0'
        return *self.ptr
    }

    func read(&mut self) : char {
        if(self.ptr >= self.end) return '\0'
        const c = *self.ptr
        self.ptr = self.ptr + 1
        self.pos += 1
        return c
    }

    func count_backticks(&self) : int {
        var c = 0;
        var p = self.ptr;
        while(p < self.end && *p == '`') {
            c += 1;
            p = p + 1;
        }
        return c;
    }

    func read_while(&mut self, pred : (c : char) => bool) {
        while(true) {
            const c = self.peek()
            if(c == '\0') break
            if(!pred(c)) break
            var _ = self.read()
        }
    }

    public func next_token(&mut self) : MdToken {
        const start_pos = self.pos
        const start_ptr = self.ptr

        if(self.in_fenced_code) {
            const bt = self.count_backticks()
            if(bt >= self.fence_count) {
                var i = 0;
                while(i < bt) { var _ = self.read(); i += 1; }
                self.in_fenced_code = false
                self.fence_count = 0
                if(self.peek() == '\r') { var _ = self.read() }
                if(self.peek() == '\n') { var _ = self.read() }
                return MdToken { type : MdTokenType.FencedCodeEnd as int, value : std::string_view("```"), position : start_pos }
            }

            while(self.peek() != '\n' && self.peek() != '\0') { var _ = self.read() }
            const end_ptr = self.ptr
            if(self.peek() == '\n') { var _ = self.read() }
            return MdToken { type : MdTokenType.CodeContent as int, value : std::string_view(start_ptr, end_ptr - start_ptr), position : start_pos }
        }

        const c = self.read()
        switch(c) {
            '\0' => { return MdToken { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : start_pos } }
            '\n' => { return MdToken { type : MdTokenType.Newline as int, value : std::string_view("\n"), position : start_pos } }
            '#' => { return MdToken { type : MdTokenType.Hash as int, value : std::string_view("#"), position : start_pos } }
            '*' => { return MdToken { type : MdTokenType.Star as int, value : std::string_view("*"), position : start_pos } }
            '_' => { return MdToken { type : MdTokenType.Underscore as int, value : std::string_view("_"), position : start_pos } }
            '[' => { return MdToken { type : MdTokenType.LBracket as int, value : std::string_view("["), position : start_pos } }
            ']' => { return MdToken { type : MdTokenType.RBracket as int, value : std::string_view("]"), position : start_pos } }
            '(' => { return MdToken { type : MdTokenType.LParen as int, value : std::string_view("("), position : start_pos } }
            ')' => { return MdToken { type : MdTokenType.RParen as int, value : std::string_view(")"), position : start_pos } }
            '!' => { return MdToken { type : MdTokenType.Exclamation as int, value : std::string_view("!"), position : start_pos } }
            '>' => { return MdToken { type : MdTokenType.GreaterThan as int, value : std::string_view(">"), position : start_pos } }
            '-' => { return MdToken { type : MdTokenType.Dash as int, value : std::string_view("-"), position : start_pos } }
            '+' => { return MdToken { type : MdTokenType.Plus as int, value : std::string_view("+"), position : start_pos } }
            '{' => { return MdToken { type : MdTokenType.LBrace as int, value : std::string_view("{"), position : start_pos } }
            '}' => { return MdToken { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : start_pos } }
            '|' => { return MdToken { type : MdTokenType.Pipe as int, value : std::string_view("|"), position : start_pos } }
            '~' => { return MdToken { type : MdTokenType.Tilde as int, value : std::string_view("~"), position : start_pos } }
            ':' => { return MdToken { type : MdTokenType.Colon as int, value : std::string_view(":"), position : start_pos } }
            '=' => { return MdToken { type : MdTokenType.Equal as int, value : std::string_view("="), position : start_pos } }
            '^' => { return MdToken { type : MdTokenType.Caret as int, value : std::string_view("^"), position : start_pos } }
            '`' => {
                if(self.peek() == '`') {
                    var _ = self.read();
                    if(self.peek() == '`') {
                        var _ = self.read();
                        var count = 3;
                        while(self.peek() == '`') { var _ = self.read(); count += 1; }
                        while(self.peek() == ' ' || self.peek() == '\t') { var _ = self.read(); }
                        const lang_start = self.ptr
                        while(self.peek() != '\n' && self.peek() != '\r' && self.peek() != '\0' && self.peek() != ' ') { var _ = self.read(); }
                        const lang_end = self.ptr
                        while(self.peek() != '\n' && self.peek() != '\0') { var _ = self.read(); }
                        if(self.peek() == '\n') { var _ = self.read(); }
                        self.in_fenced_code = true
                        self.fence_count = count
                        return MdToken { type : MdTokenType.FencedCodeStart as int, value : std::string_view(lang_start, lang_end - lang_start), position : start_pos }
                    }
                    return MdToken { type : MdTokenType.Backtick as int, value : std::string_view("``"), position : start_pos }
                }
                return MdToken { type : MdTokenType.Backtick as int, value : std::string_view("`"), position : start_pos }
            }
            '0','1','2','3','4','5','6','7','8','9' => {
                self.read_while((cc : char) => { return cc >= '0' && cc <= '9' })
                const end_ptr = self.ptr
                return MdToken { type : MdTokenType.Number as int, value : std::string_view(start_ptr, end_ptr - start_ptr), position : start_pos }
            }
            default => {
                self.read_while((cc : char) => {
                    return !(cc == '\0' || cc == '\n' || cc == '#' || cc == '*' || cc == '_' || cc == '[' || cc == ']' ||
                             cc == '(' || cc == ')' || cc == '!' || cc == '`' || cc == '>' || cc == '-' || cc == '+' ||
                             cc == '|' || cc == '~' || cc == ':' || cc == '=' || cc == '^' || cc == '{' || cc == '}' ||
                             (cc >= '0' && cc <= '9'))
                })
                const end_ptr = self.ptr
                return MdToken { type : MdTokenType.Text as int, value : std::string_view(start_ptr, end_ptr - start_ptr), position : start_pos }
            }
        }
    }

    public func lex(&mut self) : std::vector<MdToken> {
        var out = std::vector<MdToken>()
        while(true) {
            const t = next_token()
            out.push(t)
            if(t.type == MdTokenType.EndOfFile as int) break
        }
        return out
    }

}

public func lex(text : std::string_view) : std::vector<MdToken> {
    var lx = Lexer(text)
    return lx.lex();
}

}
