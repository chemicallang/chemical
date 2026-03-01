func (provider : &SourceProvider) read_identifier() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '-' || c == '_')) {
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

func (provider : &mut SourceProvider) read_line() {
    while(true) {
        const c = provider.peek()
        if(c != '\0' && c != '\r' && c != '\n') {
            provider.increment()
        } else {
            return;
        }
    }
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