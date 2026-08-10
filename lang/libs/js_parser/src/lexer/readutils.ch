public func (provider : &mut SourceProvider) read_regex_value() {
    while(true) {
        const c = provider.peek();
        if(c == '\\') {
            provider.readCharacter();
            if(provider.peek() != '\0') {
                provider.readCharacter();
            }
            continue;
        }
        if(c == '/') {
            provider.readCharacter();
            break;
        }
        if(c == '\0' || c == '\n') {
            break;
        }
        provider.readCharacter();
    }
    while(true) {
        const c = provider.peek();
        if(isalpha(c as int)) {
            provider.readCharacter();
        } else {
            break;
        }
    }
}