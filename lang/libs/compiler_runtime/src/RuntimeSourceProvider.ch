/**
 * Runtime implementations of the compiler `SourceProvider` struct methods.
 *
 * `SourceProvider` is declared as a struct (with @compiler.interface) whose
 * methods have no bodies. Inside a compiler plugin (CBI) build the compiler
 * provides these symbols. At runtime, these @no_mangle functions provide the
 * exact same mangled symbols, implemented in pure Chemical over the struct's
 * public fields.
 *
 * The runtime js package does not use a SourceProvider for its own parsing
 * (it uses JsTokenizer), but the shared js_parser package defines public
 * helper functions that reference these methods, so the symbols must exist.
 */
using namespace std;

@no_mangle
public func compiler_SourceProviderincrement(provider : *mut SourceProvider) {
    if(provider.data_ptr < provider.data_end) {
        const c = *provider.data_ptr
        provider.data_ptr += 1
        if(c == '\n') {
            provider.lineNumber += 1
            provider.lineCharacterNumber = 0
        } else {
            provider.lineCharacterNumber += 1
        }
    }
}

@no_mangle
public func compiler_SourceProviderreadCharacter(provider : *mut SourceProvider) : char {
    if(provider.data_ptr >= provider.data_end) return '\0'
    const c = *provider.data_ptr
    provider.data_ptr += 1
    if(c == '\n') {
        provider.lineNumber += 1
        provider.lineCharacterNumber = 0
    } else {
        provider.lineCharacterNumber += 1
    }
    return c
}

@no_mangle
public func compiler_SourceProviderreadCodePoint(provider : *mut SourceProvider) : u32 {
    if(provider.data_ptr >= provider.data_end) return 0
    const b0 = *provider.data_ptr
    const c0 = b0 as u32
    var cp = c0
    var len : size_t = 1
    if((c0 & 0xE0) == 0xC0 && provider.data_ptr + 1 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        cp = ((c0 & 0x1F) << 6) | ((b1 as u32) & 0x3F)
        len = 2
    } else if((c0 & 0xF0) == 0xE0 && provider.data_ptr + 2 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        const b2 = *(provider.data_ptr + 2)
        cp = ((c0 & 0x0F) << 12) | (((b1 as u32) & 0x3F) << 6) | ((b2 as u32) & 0x3F)
        len = 3
    } else if((c0 & 0xF8) == 0xF0 && provider.data_ptr + 3 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        const b2 = *(provider.data_ptr + 2)
        const b3 = *(provider.data_ptr + 3)
        cp = ((c0 & 0x07) << 18) | (((b1 as u32) & 0x3F) << 12) | (((b2 as u32) & 0x3F) << 6) | ((b3 as u32) & 0x3F)
        len = 4
    }
    var i : size_t = 0
    while(i < len) {
        compiler_SourceProviderincrement(provider)
        i += 1
    }
    return cp
}

@no_mangle
public func compiler_SourceProviderutf8_decode_peek(provider : *mut SourceProvider, out_len : &mut size_t) : u32 {
    if(provider.data_ptr >= provider.data_end) {
        *out_len = 0
        return 0
    }
    const b0 = *provider.data_ptr
    const c0 = b0 as u32
    var cp = c0
    var len : size_t = 1
    if((c0 & 0xE0) == 0xC0 && provider.data_ptr + 1 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        cp = ((c0 & 0x1F) << 6) | ((b1 as u32) & 0x3F)
        len = 2
    } else if((c0 & 0xF0) == 0xE0 && provider.data_ptr + 2 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        const b2 = *(provider.data_ptr + 2)
        cp = ((c0 & 0x0F) << 12) | (((b1 as u32) & 0x3F) << 6) | ((b2 as u32) & 0x3F)
        len = 3
    } else if((c0 & 0xF8) == 0xF0 && provider.data_ptr + 3 < provider.data_end) {
        const b1 = *(provider.data_ptr + 1)
        const b2 = *(provider.data_ptr + 2)
        const b3 = *(provider.data_ptr + 3)
        cp = ((c0 & 0x07) << 18) | (((b1 as u32) & 0x3F) << 12) | (((b2 as u32) & 0x3F) << 6) | ((b3 as u32) & 0x3F)
        len = 4
    }
    *out_len = len
    return cp
}

@no_mangle
public func compiler_SourceProviderincrementCodepoint(provider : *mut SourceProvider, cp : u32, len : size_t) {
    var i : size_t = 0
    while(i < len) {
        compiler_SourceProviderincrement(provider)
        i += 1
    }
}

@no_mangle
public func compiler_SourceProvidereof(provider : *mut SourceProvider) : bool {
    return provider.data_ptr >= provider.data_end
}

@no_mangle
public func compiler_SourceProviderpeek(provider : *mut SourceProvider) : char {
    if(provider.data_ptr >= provider.data_end) return '\0'
    return *provider.data_ptr
}

@no_mangle
public func compiler_SourceProviderincrement_char(provider : *mut SourceProvider, c : char) : bool {
    if(provider.data_ptr < provider.data_end && *provider.data_ptr == c) {
        compiler_SourceProviderincrement(provider)
        return true
    }
    return false
}

@no_mangle
public func compiler_SourceProvidergetLineNumber(provider : *mut SourceProvider) : uint {
    return provider.lineNumber
}

@no_mangle
public func compiler_SourceProvidergetLineCharNumber(provider : *mut SourceProvider) : uint {
    return provider.lineCharacterNumber
}

@no_mangle
public func compiler_SourceProviderreadWhitespaces(provider : *mut SourceProvider) : uint {
    var count : uint = 0
    var running = true
    while(running) {
        const c = compiler_SourceProviderpeek(provider)
        if(c == ' ' || c == '\t') {
            compiler_SourceProviderincrement(provider)
            count += 1
        } else {
            running = false
        }
    }
    return count
}

@no_mangle
public func compiler_SourceProviderhasNewLine(provider : *mut SourceProvider) : bool {
    const c = compiler_SourceProviderpeek(provider)
    return c == '\n' || c == '\r'
}

@no_mangle
public func compiler_SourceProviderreadNewLineChars(provider : *mut SourceProvider) : bool {
    const c = compiler_SourceProviderpeek(provider)
    if(c == '\r') {
        compiler_SourceProviderincrement(provider)
        if(compiler_SourceProviderpeek(provider) == '\n') {
            compiler_SourceProviderincrement(provider)
        }
        return true
    }
    if(c == '\n') {
        compiler_SourceProviderincrement(provider)
        return true
    }
    return false
}

@no_mangle
public func compiler_SourceProviderreadWhitespacesAndNewLines(provider : *mut SourceProvider) {
    while(true) {
        var progressed = false
        const ws = compiler_SourceProviderreadWhitespaces(provider)
        if(ws > 0) progressed = true
        const nl = compiler_SourceProviderreadNewLineChars(provider)
        if(nl) progressed = true
        if(!progressed) return
    }
}
