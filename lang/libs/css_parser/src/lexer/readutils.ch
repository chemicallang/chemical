public func (provider : &SourceProvider) read_alpha() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalpha(c as int))) {
            provider.increment();
        } else {
            break;
        }
    }
}

public func (provider : &SourceProvider) read_alpha_num() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int))) {
            provider.increment();
        } else {
            break;
        }
    }
}

public func (provider : &SourceProvider) read_css_id() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '-' || c == '_')) {
            provider.increment();
        } else {
            break;
        }
    }
}

