// Raw `/unsafe` page entry points — the `page` module's module-level `raw`
// namespace.
//
// NOTE: this cannot be spelled `page::raw` externally. Chemical resolves a
// `public namespace page` ahead of the local `page` variable that the html/css
// CBI plugins bind, which breaks `#html`/`#css` with "append_html_char ...
// required on 'page'". Keeping the namespace at module scope (`raw::…` after
// `import page`) preserves CBI generation. The CBI-required `append_*_char`
// methods likewise stay on `HtmlPage`.
//
// Everything in this namespace takes or returns a raw pointer / explicit
// length. Normal code should use the safe surface on `HtmlPage`:
//
//   page.append_html(&str)      page.append_js(&str)      page.append_css(&str)
//   page.append_head(&str)      page.append_js_end(&str)
//   escape_html(&str)           escape_html_view(string_view)
//   page.add_parameter<T>(key, &value) / add_parameter_owned / add_parameter_view
//   page.get_parameter(key)     page.has_parameter(key)
//
// The CBI macro plugins still emit calls to the `append_*_raw` / `*_char`
// methods because generated code works with `*char` + length pairs. Those
// methods stay on `HtmlPage`; this namespace is the public raw hand-off for
// callers that already own a C string.
public namespace raw {

// HTML-escape a NUL-terminated C string. Null returns "". Prefer the safe
// `escape_html(&str)`.
public func escape_html(value : *char) : std::string {
    unsafe {
        var out = std::string()
        if(value == null) { return out }
        var i : size_t = 0
        while(value[i] != '\0') {
            var c = value[i]
            if(c == '<') { out.append_view(std::string_view("&lt;")) }
            else if(c == '>') { out.append_view(std::string_view("&gt;")) }
            else if(c == '&') { out.append_view(std::string_view("&amp;")) }
            else if(c == '"') { out.append_view(std::string_view("&quot;")) }
            else if(c == '\'') { out.append_view(std::string_view("&#39;")) }
            else { out.append(c) }
            i = i + 1
        }
        return out
    }
}

// Append `len` bytes of raw markup. `value` must point to at least `len`
// readable bytes; the page copies them immediately.
public func append_html(page : &mut HtmlPage, value : *char, len : size_t) {
    page.append_html_raw(value, len)
}

// Append `len` bytes of raw JavaScript.
public func append_js(page : &mut HtmlPage, value : *char, len : size_t) {
    page.append_js_raw(value, len)
}

// Append `len` bytes of raw CSS (routed to the shared sink when attached).
public func append_css(page : &mut HtmlPage, value : *char, len : size_t) {
    page.append_css_raw(value, len)
}

// Append `len` bytes of raw `<head>` markup.
public func append_head(page : &mut HtmlPage, value : *char, len : size_t) {
    page.append_head_raw(value, len)
}

// Append `len` bytes of raw JavaScript to the post-flush tail.
public func append_js_end(page : &mut HtmlPage, value : *char, len : size_t) {
    page.append_js_end_raw(value, len)
}

// Store a caller-owned object pointer under `key`. The key view must outlive
// the page render and the pointed-to object must stay alive until the page is
// done; prefer an owned parameter for new code.
public func add_parameter_object(page : &mut HtmlPage, key : std::string_view, ptr : *mut void) {
    page.add_parameter_object_raw(key, ptr)
}

// Attach a caller-owned heap `SharedAssets` (see `shared_assets()`); the page
// does not take ownership. Prefer `attach_shared_ref`/`SharedAssetsHandle`.
public func attach_shared(page : &mut HtmlPage, assets : *mut SharedAssets) {
    page.attach_shared(assets)
}

} // namespace raw
