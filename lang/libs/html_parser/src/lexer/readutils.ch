// Parser-specific read helpers for html_parser.
//
// The generic character-class readers (read_tag_name) live in
// compiler::SourceProviderUtils so they are shared across all parsers.

/**
 * Returns true when the whitespace run between `start` and `end` contains a
 * line break.
 *
 * Such a run is pretty-printing indentation rather than content: dropping it is
 * what keeps a formatted #html block byte-identical however it is indented.
 */
public func ut_run_has_newline(start : *char, end : *char) : bool {
    var p = start;
    while(p < end) {
        if(*p == '\n' || *p == '\r') {
            return true;
        }
        p = p + 1;
    }
    return false;
}

/**
 * Returns the end of the literal text run `start .. end`, with the trailing
 * whitespace removed when that whitespace is formatting rather than content.
 *
 * A literal text run swallows whatever follows its last word, so its trailing
 * whitespace has to be judged here: the lexer's whitespace branch only runs
 * when the whitespace opens a token, and by then a run like the " " in
 * "<div>text </div>" is already part of the text token. Two trailing runs are
 * dropped:
 *
 *   * one containing a line break -- pretty-printing indentation;
 *   * one that only separates the text from a structural boundary, meaning a
 *     closing tag or the '#html' block's own '}'.
 *
 * Everything else stays. In particular the space in "See <a href=..>frame</a>"
 * is kept, because an opening tag is not a boundary: it is content that
 * separates two words, and dropping it welded them together.
 *
 * Inside <pre> (and whenever preserve_whitespace is set) nothing is dropped --
 * see pre_depth in HtmlLexer.
 */
public func ut_text_run_end(html : &mut HtmlLexer, provider : &SourceProvider, start : *char, end : *char) : *char {
    if(html.pre_depth > 0 || html.preserve_whitespace) {
        return end;
    }
    // walk back over the trailing whitespace run
    var ws_start = end;
    while(ws_start > start) {
        const c = *(ws_start - 1);
        if(c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        ws_start = ws_start - 1;
    }
    if(ws_start == end) {
        return end;
    }
    if(ut_run_has_newline(ws_start, end)) {
        return ws_start;
    }
    const next = provider.peek();
    if(next == '}') {
        return ws_start;
    }
    // only a *closing* tag is a boundary; an opening tag has content after it
    if(next == '<' && provider.data_ptr + 1 < provider.data_end && *(provider.data_ptr + 1) == '/') {
        return ws_start;
    }
    return end;
}

// reads a run of literal text
//
// Unlike read_text() this also stops at '@', because '@' begins a chemical
// construct ('@{expr}', '@if', '@else') that the lexer has to dispatch on --
// if a text run swallowed the '@' then the '{' of an "@{expr}" would be reached
// on the next call with no '@' in front of it, and inside <pre> (where a '{'
// is literal text) the interpolation would be lost.
//
// Stopping at '@' is safe for text that merely contains one: the '@' branch
// re-emits the '@' as part of a Text token when it turns out not to introduce
// a keyword, so no character is lost either way.
public func (provider : &SourceProvider) read_literal_text() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && c != '<' && c != '{' && c != '}' && c != '@') {
            provider.increment();
        } else {
            break;
        }
    }
}

// returns true if comment has ended
public func (provider : &SourceProvider) read_comment_text() : bool {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && c != '-') {
            provider.increment();
        } else {
            if(c == '-') {
                provider.increment()
                if(provider.peek() == '-') {
                    provider.increment()
                    if(provider.peek() == '>') {
                        provider.increment()
                        return true
                    }
                }
            } else {
                break;
            }
        }
    }
    return false
}

