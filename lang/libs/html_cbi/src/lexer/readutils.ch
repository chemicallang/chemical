
func (provider : &SourceProvider) read_tag_name(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
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

func (provider : &SourceProvider) read_text() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && c != '<' && c != '{' && c != '}') {
            provider.increment();
        } else {
            break;
        }
    }
}

// returns true if comment has ended
func (provider : &SourceProvider) read_comment_text() : bool {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && c != '-' && c != '{') {
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

func (provider : &SourceProvider) read_single_quoted_value() {
    while(true) {
        const c = provider.peek();
        if (c == '\'') {
            provider.increment();
            break;
        } else if(c != '\0') {
            provider.increment();
        } else {
            break;
        }
    }
}

func (provider : &SourceProvider) read_double_quoted_value() {
    while(true) {
        const c = provider.peek();
        if (c == '"') {
            provider.increment();
            break;
        } else if(c != '\0') {
            provider.increment();
        } else {
            break;
        }
    }
}

// read digits into the string
func (provider : &mut SourceProvider) read_digits() {
    while(true) {
        const next = provider.peek();
        if(isdigit(next)) {
            provider.increment();
        } else {
            break;
        }
    }
}

// assumes that a digit exists at current location
func (provider : &mut SourceProvider) read_floating_digits() : bool {
    provider.read_digits();
    const c = provider.peek();
    if(c == '.') {
        provider.increment();
        provider.read_digits();
        return true;
    } else {
        return false;
    }
}

func (provider : &SourceProvider) skip_whitespaces() {
    while(true) {
        const c = provider.peek();
        switch(c) {
            ' ', '\t', '\n', '\r' => {
                provider.readCharacter();
            }
            default => {
                return;
            }
        }
    }
}