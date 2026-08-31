/**
 * Shared HTML utilities extracted from html_cbi and html runtime.
 */

using namespace std;

public func html_is_entity(text : std::string_view, index : uint) : bool {
    if (index + 2 >= text.size()) return false
    if (text.data()[index] != '&') return false

    var i = index + 1
    if (text.data()[i] == '#') {
        i++
        if (i < text.size() && (text.data()[i] == 'x' || text.data()[i] == 'X')) {
            i++
            var start = i
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i]
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) { i++ } else break
            }
            return (i > start && i < text.size() && text.data()[i] == ';')
        } else {
            var start = i
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i]
                if (c >= '0' && c <= '9') { i++ } else break
            }
            return (i > start && i < text.size() && text.data()[i] == ';')
        }
    } else {
        var start = i
        while (i < text.size() && i - start < 32) {
            const c = text.data()[i]
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) { i++ } else break
        }
        return (i > start && i < text.size() && text.data()[i] == ';')
    }
}

public func html_escape_append(str : &mut std::string, text : std::string_view) {
    var i = 0u
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF
        if (c1 < 0x80) {
            const c = c1 as char
            switch(c) {
                '&' => {
                    if (html_is_entity(text, i)) { str.append('&') } else { str.append_view("&amp;") }
                }
                '<' => str.append_view("&lt;")
                '>' => str.append_view("&gt;")
                '"' => str.append_view("&quot;")
                '\'' => str.append_view("&#39;")
                default => str.append(c)
            }
            i++
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F)
                str.append_view("&#"); str.append_uinteger(codepoint as ubigint); str.append(';')
                i += 2
            } else { i++ }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)
                str.append_view("&#"); str.append_uinteger(codepoint as ubigint); str.append(';')
                i += 3
            } else { i++ }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const c4 = (text.data()[i+3] as uint) & 0xFF
                const codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)
                str.append_view("&#"); str.append_uinteger(codepoint as ubigint); str.append(';')
                i += 4
            } else { i++ }
        } else { i++ }
    }
}
