/**
 * Shared `SourceProvider` read-helper functions.
 *
 * These generic text-scanning helpers are used by every parser package
 * (js_parser, html_parser, css_parser, universal_parser). They are defined
 * once here, in the `compiler` package that all parser packages import, so
 * that multiple parsers can be linked into a single program (e.g. the test
 * executable) without duplicate-symbol conflicts.
 *
 * Inside a CBI plugin the `SourceProvider` methods (peek, increment,
 * readCharacter, ...) are provided by the compiler; at runtime they are
 * provided by the compiler_runtime package.
 */

public func (provider : &SourceProvider) read_identifier() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '-' || c == '_' || c == '$')) {
            provider.increment();
        } else {
            break;
        }
    }
}

public func (provider : &mut SourceProvider) read_line() {
    while(true) {
        const c = provider.peek()
        if(c != '\0' && c != '\r' && c != '\n') {
            provider.increment()
        } else {
            return;
        }
    }
}

public func (provider : &SourceProvider) read_text() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && c != '<' && c != '{' && c != '}') {
            provider.increment();
        } else {
            break;
        }
    }
}

public func (provider : &SourceProvider) read_single_quoted_value() {
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

public func (provider : &SourceProvider) read_double_quoted_value() {
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
public func (provider : &mut SourceProvider) read_digits() {
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
public func (provider : &mut SourceProvider) read_floating_digits() : bool {
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

public func (provider : &SourceProvider) skip_whitespaces() {
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

// ─── Character-class readers ─────────────────────────────────────────────────
// Shared readers for common character classes used by multiple parsers.
// Each reader consumes characters matching its predicate until the first
// non-matching character is encountered.

/** Read consecutive alphabetic characters (a-z, A-Z). */
public func (provider : &SourceProvider) read_alpha() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && isalpha(c as int)) {
            provider.increment();
        } else {
            break;
        }
    }
}

/** Read consecutive alphanumeric characters (a-z, A-Z, 0-9). */
public func (provider : &SourceProvider) read_alpha_num() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && isalnum(c as int)) {
            provider.increment();
        } else {
            break;
        }
    }
}

/**
 * Read a CSS-style identifier: alphanumeric characters, hyphens, and underscores.
 * Used by css_parser for selectors, property names, and class names.
 */
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

/**
 * Read an HTML tag or attribute name: alphanumeric characters, underscores,
 * hyphens, and colons. Used by html_parser for tag/attribute lexing.
 */
public func (provider : &SourceProvider) read_tag_name() {
    while(true) {
        const c = provider.peek();
        if(c != '\0' && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            provider.increment();
        } else {
            break;
        }
    }
}
