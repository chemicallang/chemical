
func ut_char_lower_ascii(c : char) : char {
    if(c >= 'A' && c <= 'Z') {
        return ((c as int) + 32) as char
    }
    return c
}

// Returns true when the provider is positioned at a case-insensitive
// "</script" followed by a tag terminator. Used to end the raw script text.
func ut_at_script_close(provider : &SourceProvider) : bool {
    // need bytes [0..8] available ('<' '/' 's' 'c' 'r' 'i' 'p' 't' and the terminator)
    if(provider.data_ptr + 9 > provider.data_end) {
        return false;
    }
    const p = provider.data_ptr
    if(*(p + 0) != '<') { return false; }
    if(*(p + 1) != '/') { return false; }
    if(ut_char_lower_ascii(*(p + 2)) != 's') { return false; }
    if(ut_char_lower_ascii(*(p + 3)) != 'c') { return false; }
    if(ut_char_lower_ascii(*(p + 4)) != 'r') { return false; }
    if(ut_char_lower_ascii(*(p + 5)) != 'i') { return false; }
    if(ut_char_lower_ascii(*(p + 6)) != 'p') { return false; }
    if(ut_char_lower_ascii(*(p + 7)) != 't') { return false; }
    const after = *(p + 8)
    return after == '>' || after == '/' || after == ' ' || after == '\t' || after == '\n' || after == '\r'
}

func is_script_tag_name(tag_value : std::string_view) : bool {
    return tag_value.size() == 6 &&
        ut_char_lower_ascii(tag_value.get(0)) == 's' &&
        ut_char_lower_ascii(tag_value.get(1)) == 'c' &&
        ut_char_lower_ascii(tag_value.get(2)) == 'r' &&
        ut_char_lower_ascii(tag_value.get(3)) == 'i' &&
        ut_char_lower_ascii(tag_value.get(4)) == 'p' &&
        ut_char_lower_ascii(tag_value.get(5)) == 't'
}

public func getNextToken2(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    const provider = &mut lexer.provider;
    // Inside <script>, everything up to </script> is raw text (JS is not HTML).
    // Emit it as one Text token; when the closing tag is reached, clear the mode
    // and fall through so the '<' is lexed as a normal end tag.
    if(html.in_script) {
        const position = provider.getPosition();
        const start = provider.current_data();
        while(!provider.eof()) {
            if(ut_at_script_close(provider)) {
                break;
            }
            provider.readCharacter();
        }
        html.in_script = false;
        const written = provider.current_data() - start;
        if(written > 0) {
            return Token {
                type : TokenType.Text as int,
                value : std::string_view(start, written),
                position : position
            }
        }
        // empty script body: fall through and lex "</script>"
    }
    // the position of the current symbol
    const position = provider.getPosition();
    const data_ptr = provider.current_data()
    const c = provider.readCharacter();
    switch(c) {
        '\0' => {
            return Token {
                type : TokenType.EndOfFile as int,
                value : view(""),
                position : position
            }
        }
        '<' => {
            const p = provider.peek()
            if(p == '!') {
                provider.readCharacter();
                const next = provider.peek()
                if(next == '-') {

                    // its a comment
                    provider.readCharacter()
                    if(provider.peek() == '-') {
                        provider.readCharacter()
                    }
                    html.is_comment = true;

                    return Token {
                        type : TokenType.CommentStart as int,
                        value : view("<!--"),
                        position : position
                    }

                } else if(isalpha(next)) {

                    // its a directive
                    // TODO handle this case
                    return Token {
                        type : TokenType.Unexpected as int,
                        value : view("expected directive or comment"),
                        position : position
                    }

                } else {
                    return Token {
                        type : TokenType.Unexpected as int,
                        value : view("expected directive or comment"),
                        position : position
                    }
                }
            } else if(p == '/') {
                html.has_lt = true;
                html.in_end_tag = true;
                html.pending_script = false;
                provider.readCharacter();
                return Token {
                    type : TokenType.TagEnd as int,
                    value : view("</"),
                    position : position
                }
            } else if(isalpha(p as int)) {
                html.has_lt = true;
                return Token {
                    type : TokenType.LessThan as int,
                    value : view("<"),
                    position : position
                }
            } else {
                // A '<' that cannot begin a tag name, an end tag or a markup
                // declaration is ordinary text. This is the HTML "tag open"
                // state: a browser enters tag mode only for '!', '/' and an
                // ASCII letter, and renders every other '<' literally. So
                // "1 < 2" must be text, not a tag named "2"; without this the
                // whole #html block fails to parse, because the parser sees a
                // raw number token where it expected text or an element.
                const start = data_ptr;
                provider.read_literal_text();
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            }
        }
        '@' => {
            if(provider.peek() == '{') {
                provider.readCharacter();
                html.other_mode = true;
                html.chemical_mode = true;
                html.chem_start_lb = html.lb_count;
                html.lb_count++;
                return Token {
                    type : TokenType.ChemicalNodeStart as int,
                    value : view("@{"),
                    position : position
                }
            } else if(provider.peek() == '(') {
                // "@(expr)" is the explicit form of a chemical value. It works
                // everywhere, and inside <pre> it is the only way to write one,
                // because there a bare '{' is a literal character.
                //
                // The '(' is consumed here rather than left for the chemical
                // lexer (which is what the "@if(" path does), so paren_count
                // starts at 1 to account for it. lb_count is deliberately left
                // alone: this region ends at its matching ')' and must not
                // disturb the brace depth, which is what lets it appear inside
                // <pre> where '}' is text.
                provider.readCharacter();
                html.other_mode = true;
                html.chemical_mode = true;
                html.in_paren_value = true;
                html.paren_count = 1;
                return Token {
                    type : TokenType.ChemicalValueStart as int,
                    value : view("@("),
                    position : position
                }
            } else if(!html.has_lt && isalpha(provider.peek() as int)) {
                
                // then we have a keyword, lets parse it
                const start = provider.current_data(); 
                provider.read_tag_name();
                const value = std::string_view(start, provider.current_data() - start);
                const hash = fnv1_hash_view(&value);

                switch(hash) {
                    comptime_fnv1_hash("if") => {
                         provider.skip_whitespaces();
                         if(provider.peek() == '(') {
                            html.other_mode = true;
                            html.chemical_mode = true;
                            html.in_paren_expr = true;
                            html.paren_count = 0;
                        }
                        return Token {
                            type : TokenType.If as int,
                            value : value,
                            position : position
                        }
                    }
                    comptime_fnv1_hash("else") => {
                        html.expecting_html_block = true;
                        return Token {
                            type : TokenType.Else as int,
                            value : value,
                            position : position
                        }
                    }
                    default => {
                        // '@' followed by a name that is not a keyword is
                        // ordinary text: a decorator, an annotation, an e-mail
                        // address. Keep reading the text run from here rather
                        // than stopping at the end of the name, otherwise the
                        // whitespace between this word and the text after it is
                        // split off and then dropped as insignificant, which
                        // turned "@Override and @app" into "@Overrideand".
                        provider.read_literal_text();
                        return Token {
                            type : TokenType.Text as int,
                            value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                            position : position
                        }
                    }
                }

            } else {
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            }
        }
        '}' => {
            if(html.pre_depth > 0 &&
                (html.pre_brace_depth > 0 || html.lb_count <= 1)) {
                // Inside <pre> a '}' is literal text in two cases:
                //
                //  * pre_brace_depth > 0 -- it closes a '{' that was itself
                //    read as literal text, so the two pair with each other and
                //    lb_count is never disturbed;
                //  * lb_count <= 1 -- no html block is open, so this '}' cannot
                //    be closing one. It is NOT the '}' that ends the #html
                //    macro either: the macro's own '}' comes after the
                //    </pre>, where pre_depth is already back to 0.
                //
                // Reading a stray '}' as the macro close is what used to drop
                // the remainder of the file out of the macro.
                if(html.pre_brace_depth > 0) {
                    html.pre_brace_depth--;
                }
                const start = data_ptr;
                provider.read_literal_text();
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            }
            if(html.lb_count == 1) {
                html.reset();
                lexer.unsetUserLexer();
            } else {
                html.lb_count--;
            }
            return Token {
                type : TokenType.RBrace as int,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            if(html.pre_depth > 0 && !html.expecting_html_block) {
                // Inside <pre> the text is displayed verbatim and code is full
                // of braces, so a '{' that does not open an @if/@else html
                // block is literal text rather than the start of a chemical
                // value. Interpolation is still available in the explicit
                // "@{expr}" form, and @if/@else blocks still work, because
                // those set expecting_html_block and are matched by this
                // branch not being taken.
                //
                // Counting it keeps literal braces paired with each other so a
                // '}' later in the same <pre> can be recognised as literal too
                // (see the '}' case).
                html.pre_brace_depth++;
                const start = data_ptr;
                provider.read_literal_text();
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            }
            if(html.lb_count >= 1 && !html.expecting_html_block) {
                html.other_mode = true;
                html.chemical_mode = true;
                html.chem_start_lb = html.lb_count;
            }
            html.lb_count++;
            html.expecting_html_block = false;
            return Token {
                type : TokenType.LBrace as int,
                value : view("{"),
                position : position
            }
        }
        ' ', '\t', '\n', '\r' => {
            if(!html.has_lt) {
                const was_after_chem = html.after_chem_expr;
                html.after_chem_expr = false;
                provider.skip_whitespaces();
                const next = provider.peek();
                if(html.pre_depth > 0 || html.preserve_whitespace) {
                    // inside <pre> (or when preserve_whitespace is on),
                    // whitespace-only runs between elements are significant;
                    // they must survive unless they precede a chemical
                    // statement ('@...') or the enclosing block/macro close
                    // ('}'), or an html block is expected (right after
                    // '@if(...)' or '@else', where whitespace is a separator)
                    //
                    // A '}' that is read back as a literal brace of <pre> code --
                    // because a literal '{' is still unmatched, which is what the
                    // '{' branch counted -- is content, not a close, so only a
                    // '}' that really ends a chemical block or expression is a
                    // boundary. See the matching '{' case below.
                    const literal_brace = html.pre_depth > 0 && (html.pre_brace_depth > 0 || html.lb_count <= 1);
                    const closes_chem = next == '}' && !literal_brace;
                    if(next != '@' && !closes_chem && !html.expecting_html_block) {
                        return Token {
                            type : TokenType.Text as int,
                            value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                            position : position
                        }
                    }
                    return getNextToken2(html, lexer);
                }
                // after a chemical expression we preserve the whitespace that
                // separates it from following content (e.g. "{value} World"),
                // but drop whitespace that only precedes a structural boundary
                // like a closing tag "</...>" or the html macro's closing "}"
                // (e.g. "<div>{x}Text</div>\n    }")
                const is_boundary = next == '<' || next == '}';
                if(was_after_chem && !is_boundary) {
                    return Token {
                        type : TokenType.Text as int,
                        value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                        position : position
                    }
                }
                // A run of spaces and tabs that stays on the line it started on is
                // content, and dropping it welded words together -- "</a>is" for
                // "</a> is". Runs that are still dropped, which is what keeps every
                // pretty-printed block byte-identical and is pinned by
                // lang/tests/compiler_plugins/html/src/whitespace_after_tag.ch:
                //
                //   * a run containing a newline or carriage return, because that
                //     is indentation rather than a space somebody typed;
                //   * a run at a structural boundary -- before a tag, before the
                //     macro's own '}', or before a chemical construct. That last
                //     one covers '{' and '@', which open an expression or a block
                //     whose value follows, so the run only separates them. The
                //     macro's own opening brace ("#html {") is read through this
                //     very branch, which is why it has to be dropped here.
                const opens_chem = next == '{' || next == '@' || html.expecting_html_block;
                if(!is_boundary && !opens_chem &&
                    !ut_run_has_newline(data_ptr, provider.current_data())) {
                    return Token {
                        type : TokenType.Text as int,
                        value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                        position : position
                    }
                }
                return getNextToken2(html, lexer);
            }
            provider.skip_whitespaces();
            return getNextToken2(html, lexer);
        }
        '/' => {
            if(html.has_lt) {
                // self-closing <pre/> : we are leaving the pre context
                if(html.last_tag_pre) {
                    html.pre_depth--;
                    html.last_tag_pre = false;
                }
                if(html.pre_depth == 0) {
                    // leaving <pre>: an unbalanced literal '{' must not leak
                    // past the closing tag
                    html.pre_brace_depth = 0;
                }
                // <script/> is self-closing, not a raw-text element
                html.pending_script = false;
                return Token {
                    type : TokenType.FwdSlash as int,
                    value : view("/"),
                    position : position
                }
            } else {
                const start = data_ptr;
                provider.read_literal_text()
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            }
        }
        '>' => {
            if(html.has_lt) {
                // reset back
                html.has_lt = false;
                html.lexed_tag_name = false;
                html.in_end_tag = false;
                html.last_tag_pre = false;
                if(html.pending_script) {
                    // we just opened a <script>; its content is raw text
                    html.pending_script = false;
                    html.in_script = true;
                }
                return Token {
                    type : TokenType.GreaterThan as int,
                    value : view(">"),
                    position : position
                }
            } else {
                const start = data_ptr;
                provider.read_literal_text()
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            }
        }
        default => {
            if(html.has_lt) {
                if(isalpha(c as int)) {
                    if(html.lexed_tag_name) {
                        provider.read_tag_name();
                        return Token {
                            type : TokenType.AttrName as int,
                            value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                            position : position
                        }
                    } else {
                        html.lexed_tag_name = true;
                        provider.read_tag_name();
                        const tag_value = std::string_view(data_ptr, provider.current_data() - data_ptr);
                        if(!html.in_end_tag) {
                            const is_pre = tag_value.size() == 3 &&
                                tag_value.get(0) == 'p' &&
                                tag_value.get(1) == 'r' &&
                                tag_value.get(2) == 'e';
                            if(is_pre) {
                                html.pre_depth++;
                            }
                            html.last_tag_pre = is_pre;
                            html.pending_script = is_script_tag_name(tag_value);
                        } else {
                            // closing tag; match against the currently open <pre>
                            const is_pre_close = html.pre_depth > 0 && tag_value.size() == 3 &&
                                tag_value.get(0) == 'p' &&
                                tag_value.get(1) == 'r' &&
                                tag_value.get(2) == 'e';
                            if(is_pre_close) {
                                html.pre_depth--;
                            }
                            if(html.pre_depth == 0) {
                                // leaving <pre>: an unbalanced literal '{' must
                                // not leak past the closing tag
                                html.pre_brace_depth = 0;
                            }
                            html.pending_script = false;
                            html.in_end_tag = false;
                        }
                        return Token {
                            type : TokenType.TagName as int,
                            value : tag_value,
                            position : position
                        }
                    }
                } else {
                    switch(c) {
                        '=' => {
                           return Token {
                                type : TokenType.Equal as int,
                                value : view("="),
                                position : position
                           }
                        }
                        '\'' => {
                            provider.read_single_quoted_value()
                            return Token {
                                type : TokenType.SingleQuotedValue as int,
                                value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                                position : position
                            }
                        }
                        '"' => {
                            provider.read_double_quoted_value()
                            return Token {
                                type : TokenType.DoubleQuotedValue as int,
                                value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                                position : position
                            }
                        }
                        default => {
                            if(isdigit(c)) {
                                provider.read_floating_digits();
                                return Token {
                                    type : TokenType.Number as int,
                                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                                    position : position
                                }
                            } else {
                                return Token {
                                    type : TokenType.Unexpected as int,
                                    value : view("tag names must start with letters"),
                                    position : position
                                }
                            }
                        }
                    }
                }
            } else {
                const start = data_ptr;
                provider.read_literal_text()
                // the run swallowed the whitespace after its last word, so that
                // trailing part is judged here rather than by the whitespace
                // branch above, which only runs when whitespace opens a token
                const run_end = ut_text_run_end(html, provider, start, provider.current_data());
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(start, run_end - start),
                    position : position
                }
            }
        }
    }
}