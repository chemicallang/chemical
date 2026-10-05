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

// ===== <script>: raw text vs. interpolatable =====
//
// A <script> is lexed in a raw-text mode (everything up to </script> is one
// Text token, emitted verbatim with no HTML entity escaping) because JavaScript
// is not HTML and must not be lexed as HTML -- `if (x) { ... }` is full of
// braces that are not chemical syntax.
//
// That justification is about the *content*, so the mode has to be keyed on the
// content and not on the element name alone. Keying it on the name also captured
// the one very common non-JS use of <script>: a data island, most often
// `<script type="application/json">`. There, '{' was never offered to the
// parser, so an interpolated value was emitted as its literal source text with
// no diagnostic anywhere -- the page compiled, served and returned 200, and
// merely contained "{payload}" where the payload should have been.
//
// The lexer (which decides whether to enter raw-text mode) and the converter
// (which decides whether to emit a script body verbatim) deliberately share this
// one predicate. Having them agree on something weaker than "is this JS" is how
// the element name ended up being the only thing they agreed on at all.

public func html_lower_byte(c : char) : char {
    if(c >= 'A' && c <= 'Z') { return ((c as int) + 32) as char }
    return c
}

// Attribute values arrive as source text INCLUDING their delimiters: the
// lexer's SingleQuotedValue / DoubleQuotedValue tokens span the quotes, so
// `type="module"` lexes to the eight characters `"module"`. Classifying one
// therefore means comparing the unquoted value. A Number token has no
// delimiters and passes through unchanged.
public func html_unquote_attribute_value(value : std::string_view) : std::string_view {
    if(value.size() >= 2u) {
        const first = value.data()[0]
        const last = value.data()[value.size() - 1u]
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            return std::string_view(value.data() + 1, value.size() - 2u)
        }
    }
    return value
}

// ASCII case-insensitive comparison of two views. Attribute names and MIME types
// are matched this way, so a case-sensitive compare would let
// `Type="Application/JSON"` slip into the raw path.
public func html_view_iequals(a : std::string_view, b : std::string_view) : bool {
    if(a.size() != b.size()) { return false }
    var i = 0u
    while(i < a.size()) {
        if(html_lower_byte(a.get(i)) != html_lower_byte(b.get(i))) { return false }
        i = i + 1u
    }
    return true
}

public func html_view_istarts_with(v : std::string_view, prefix : std::string_view) : bool {
    if(v.size() < prefix.size()) { return false }
    var i = 0u
    while(i < prefix.size()) {
        if(html_lower_byte(v.get(i)) != html_lower_byte(prefix.get(i))) { return false }
        i = i + 1u
    }
    return true
}

public func html_view_ends_with(v : std::string_view, suffix : std::string_view) : bool {
    if(v.size() < suffix.size()) { return false }
    return html_view_iequals(v.subview(v.size() - suffix.size(), v.size()), suffix)
}

// True when a `type` attribute value denotes script data -- JavaScript, a module,
// or a script-adjacent JSON document that a browser also parses as script data.
// Everything else (application/json, application/ld+json, text/template, ...) is
// a data block whose content is NOT JavaScript and therefore must go through the
// ordinary text/interpolation path.
public func html_script_type_is_script_data(type_value : std::string_view) : bool {
    // An absent or empty `type` means JavaScript.
    if(type_value.size() == 0u) { return true }
    if(html_view_iequals(type_value, std::string_view("module"))) { return true }
    // Script-adjacent JSON. Not JavaScript, but a browser parses these as script
    // data rather than as a data block, and their content is full of '{' --
    // routing them through the interpolating path would make every one of those
    // a chemical expression.
    if(html_view_iequals(type_value, std::string_view("importmap"))) { return true }
    if(html_view_iequals(type_value, std::string_view("speculationrules"))) { return true }
    // A JavaScript MIME type: the HTML "JavaScript MIME type essence match"
    // narrowed to the spellings that actually identify JavaScript, plus the
    // historical ".../javascript" forms.
    if(html_view_istarts_with(type_value, std::string_view("application/javascript"))) { return true }
    if(html_view_istarts_with(type_value, std::string_view("text/javascript"))) { return true }
    if(html_view_istarts_with(type_value, std::string_view("application/ecmascript"))) { return true }
    if(html_view_istarts_with(type_value, std::string_view("text/ecmascript"))) { return true }
    if(html_view_ends_with(type_value, std::string_view("/javascript"))) { return true }
    if(html_view_ends_with(type_value, std::string_view("/ecmascript"))) { return true }
    return false
}
