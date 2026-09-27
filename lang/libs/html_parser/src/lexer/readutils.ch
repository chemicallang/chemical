// Parser-specific read helpers for html_parser.
//
// The generic character-class readers (read_tag_name) live in
// compiler::SourceProviderUtils so they are shared across all parsers.

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

