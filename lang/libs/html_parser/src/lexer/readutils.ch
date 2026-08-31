// Parser-specific read helpers for html_parser.
//
// The generic character-class readers (read_tag_name) live in
// compiler::SourceProviderUtils so they are shared across all parsers.

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

