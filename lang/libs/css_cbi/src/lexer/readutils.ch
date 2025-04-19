
func (provider : &SourceProvider) read_tag_name(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_alpha(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalpha(c as int))) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_alpha_num(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalnum(c as int))) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_css_id(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalnum(c as int) || c == '-' || c == '_')) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_text(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && c != '<' && c != '{' && c != '}') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_single_quoted_value(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if (c == '\'') {
            str.append(provider.readCharacter());
            break;
        } else if(c != -1) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_double_quoted_value(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if (c == '"') {
            str.append(provider.readCharacter());
            break;
        } else if(c != -1) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

// read digits into the string
func (provider : &mut SourceProvider) read_digits(str : &mut SerialStrAllocator) {
    while(true) {
        const next = provider.peek();
        if(next != -1 && isdigit(next)) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

func (provider : &mut SourceProvider) read_line(str : &mut SerialStrAllocator) {
    while(true) {
        const c = provider.peek()
        if(c != -1 && c != '\r' && c != '\n') {
            str.append(provider.readCharacter())
        } else {
            return;
        }
    }
}

// assumes that a digit exists at current location
func (provider : &mut SourceProvider) read_floating_digits(str : &mut SerialStrAllocator) : bool {
    provider.read_digits(str);
    const c = provider.peek();
    if(c == '.') {
        str.append(provider.readCharacter());
        provider.read_digits(str);
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