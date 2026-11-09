// safe — pointer-free façade over the page API.
//
// The canonical normal-use surface is on `HtmlPage` itself:
//
//   page.append_html(&str)      page.append_js(&str)      page.append_css(&str)
//   page.append_head(&str)      page.append_js_end(&str)
//   escape_html(&str)           escape_html_view(string_view)
//   page.add_parameter<T>(key, &value)
//   page.add_parameter_owned / add_parameter_view
//   page.get_parameter(key) / has_parameter(key)
//
// Raw `*char` + length entry points (and the CBI-required `append_*_char`
// helpers) live in the page module's `raw` namespace (`raw::…`) and on
// `HtmlPage` as `append_*_raw`.
//
// The flat `safe_page_*` functions below are compatibility aliases for the
// original P2.5 spellings. New code should use the methods.
//
// They deliberately do NOT live in a namespace named `page`: such a namespace
// shadows the module name and makes the html/css CBI plugins fail to find the
// `HtmlPage` helpers (`append_html_char`, `append_css_char`, …).
//
// `append_html` / `append_js` append the bytes VERBATIM (they are
// markup/script sinks). Use `escape_html` / `safe_page_escape_js` for
// untrusted data. `safe_page_escape_js` additionally escapes `'`, `\`, control
// characters and `</` (so a value can never close an inline <script> element).

using std::Option;
using std::string;
using std::string_view;

// ---------------------------------------------------------------------------
// Escaping
// ---------------------------------------------------------------------------

// HTML-escape untrusted text (`& < > " '`). The result owns its bytes.
public func safe_page_escape_html(value : &std::str) : string {
    var view = value.to_view()
    var out = string()
    appendHtmlEscaped(&mut out, &view)
    return out
}

// The same for a `string_view`.
public func safe_page_escape_html_view(value : string_view) : string {
    var out = string()
    appendHtmlEscaped(&mut out, &value)
    return out
}

// JS string-literal escaping. Escapes `"`, `'`, `\`, newlines/tabs/control
// characters, and `</` (emitted as `\u003C/`), so the value is safe inside a
// single- OR double-quoted JS string and cannot break out of an inline
// <script>. The result is the escaped body only (caller adds quotes).
public func safe_page_escape_js(value : &std::str) : string {
    var view = value.to_view()
    var out = string()
    var i : size_t = 0
    while(i < view.size()) {
        var c = view.at(i)
        if(c == '"') {
            out.append_view(std::string_view("\\\""))
        } else if(c == '\'') {
            out.append_view(std::string_view("\\'"))
        } else if(c == '\\') {
            out.append_view(std::string_view("\\\\"))
        } else if(c == '\n') {
            out.append_view(std::string_view("\\n"))
        } else if(c == '\r') {
            out.append_view(std::string_view("\\r"))
        } else if(c == '\t') {
            out.append_view(std::string_view("\\t"))
        } else if(c == '<') {
            if(i + 1 < view.size() && view.at(i + 1) == '/') {
                out.append_view(std::string_view("\\u003C/"))
                i = i + 1
            } else {
                out.append(c)
            }
        } else if((c as u8) < 0x20u8) {
            out.append_view(std::string_view("\\u00"))
            appendJsHex2(&mut out, c as uint)
        } else {
            out.append(c)
        }
        i = i + 1
    }
    return out
}

// JSON string-body escaping (`"`, `\`, control characters, `</`). JSON does
// not require escaping `'`; `safe_page_escape_js` is the stricter form for
// inline JS.
public func safe_page_escape_json(value : &std::str) : string {
    var view = value.to_view()
    var out = string()
    var i : size_t = 0
    while(i < view.size()) {
        var c = view.at(i)
        if(c == '"') {
            out.append_view(std::string_view("\\\""))
        } else if(c == '\\') {
            out.append_view(std::string_view("\\\\"))
        } else if(c == '\n') {
            out.append_view(std::string_view("\\n"))
        } else if(c == '\r') {
            out.append_view(std::string_view("\\r"))
        } else if(c == '\t') {
            out.append_view(std::string_view("\\t"))
        } else if(c == '<') {
            if(i + 1 < view.size() && view.at(i + 1) == '/') {
                out.append_view(std::string_view("\\u003C/"))
                i = i + 1
            } else {
                out.append(c)
            }
        } else if((c as u8) < 0x20u8) {
            out.append_view(std::string_view("\\u00"))
            appendJsHex2(&mut out, c as uint)
        } else {
            out.append(c)
        }
        i = i + 1
    }
    return out
}

// ---------------------------------------------------------------------------
// Buffer appends
// ---------------------------------------------------------------------------

// Append raw HTML (a length-tracked `&str`, no NUL scan).
public func safe_page_append_html(page : &mut HtmlPage, value : &std::str) {
    page.append_html(value)
}

// Append raw JavaScript.
public func safe_page_append_js(page : &mut HtmlPage, value : &std::str) {
    page.append_js(value)
}

// Append HTML-escaped text.
public func safe_page_append_html_escaped(page : &mut HtmlPage, value : &std::str) {
    var escaped = safe_page_escape_html(value)
    var escaped_str = std::str.make_view(escaped.to_view())
    page.append_html(&escaped_str)
}

// Append a JS-escaped string BODY (without surrounding quotes).
public func safe_page_append_js_escaped(page : &mut HtmlPage, value : &std::str) {
    var escaped = safe_page_escape_js(value)
    var escaped_str = std::str.make_view(escaped.to_view())
    page.append_js(&escaped_str)
}

// ---------------------------------------------------------------------------
// Typed page parameters
// ---------------------------------------------------------------------------

// A value that can be stored as page-owned parameter text. Primitive numeric
// types, booleans, chars, and the owned/borrowed string types implement it.
public interface SafePageParam {
    public func to_page_param(&self) : string
}

impl SafePageParam for int {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_integer(*self)
        return out
    }
}

impl SafePageParam for uint {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_uinteger(*self)
        return out
    }
}

impl SafePageParam for i8 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_integer(*self)
        return out
    }
}

impl SafePageParam for i16 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_integer(*self)
        return out
    }
}

impl SafePageParam for i32 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_integer(*self)
        return out
    }
}

impl SafePageParam for i64 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_integer(*self)
        return out
    }
}

impl SafePageParam for u8 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_uinteger(*self)
        return out
    }
}

impl SafePageParam for u16 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_uinteger(*self)
        return out
    }
}

impl SafePageParam for u32 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_uinteger(*self)
        return out
    }
}

impl SafePageParam for u64 {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_uinteger(*self)
        return out
    }
}

impl SafePageParam for bool {
    public func to_page_param(&self) : string {
        if(*self) { return string("true") }
        return string("false")
    }
}

impl SafePageParam for char {
    public func to_page_param(&self) : string {
        var out = string()
        out.append(*self)
        return out
    }
}

impl SafePageParam for double {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_double(*self, 3)
        return out
    }
}

impl SafePageParam for float {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_float(*self, 3)
        return out
    }
}

impl SafePageParam for string {
    public func to_page_param(&self) : string {
        return self.copy()
    }
}

impl SafePageParam for string_view {
    public func to_page_param(&self) : string {
        var out = string()
        out.append_view(self)
        return out
    }
}

impl SafePageParam for std::str {
    public func to_page_param(&self) : string {
        var out = string()
        var view = self.to_view()
        out.append_view(&view)
        return out
    }
}

// Store a page-owned copy of `value` under `key`. Both the key and the value
// remain valid for the rest of the render (unlike `add_parameter`, whose
// `string_view` arguments must outlive the page).
public func <T : SafePageParam> safe_page_set_parameter(page : &mut HtmlPage, key : &std::str, value : &T) {
    var text = value.to_page_param()
    page.add_parameter_owned(key.to_view(), text.to_view())
}

// The parameter text as an owned string, or None when absent.
public func safe_page_get_parameter_safe(page : &HtmlPage, key : &std::str) : Option<string> {
    var k = key.to_view()
    if(!page.has_parameter(k)) {
        return Option.None<string>()
    }
    var view = page.get_parameter(k)
    var out = string()
    out.append_view(&view)
    return Option.Some<string>(out)
}
