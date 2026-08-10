
func (provider : &SourceProvider) read_tag_name() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            provider.increment();
        } else {
            break;
        }
    }
}

func (provider : &SourceProvider) read_attr_name() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
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
                        return true;
                    }
                }
            } else {
                break;
            }
        }
    }
    return false
}




