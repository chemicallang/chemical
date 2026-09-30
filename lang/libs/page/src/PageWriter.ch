// Two lowercase hex digits. `appendJsHex2` in ssr.ch is the same helper; it is
// duplicated rather than shared because the two files do not see each other's
// module-private functions.
func append_page_writer_hex2(buffer : &mut std::string, v : uint) {
    const hex = "0123456789abcdef"
    buffer.append(hex[(v >> 4) & 0xF]);
    buffer.append(hex[v & 0xF]);
}

// Escapes `data` for a single-quoted JavaScript string literal and appends it.
//
// This output lands in `pageJs`, which is emitted inside an **inline <script>**
// element. A script element's raw text ends at the first `</script` regardless
// of the JS string context, so an unescaped `</` in an interpolated value closes
// the block and whatever follows it runs as script. A single quote in the value
// breaks out of the literal too.
//
// This is the sink that matters. `html_cbi`'s `emit_universal_queue` asks
// `is_string_type` whether to add its own quotes, and a `std::string` /
// `std::string_view` value is a *Linked* type, so the answer is false and the
// value is delegated here instead. That is why the two
// `append_escaped_single_quoted` helpers (html_cbi, universal_cbi) look like the
// right place to fix this and are not: they have no callers.
//
// `\u003C` is the escape for `<` and decodes back to `<` inside a JS string, so
// the value is unchanged. Mirrors `appendJsEscaped` (ssr.ch), which is the
// double-quoted twin and additionally leaves `'` alone.
func append_js_single_quoted_escaped(buffer : &mut std::string, data : *char, len : size_t) {
    var i : size_t = 0
    while(i < len) {
        const c = data[i]
        switch(c) {
            '\\' => buffer.append_view("\\\\")
            '\'' => buffer.append_view("\\'")
            '\n' => buffer.append_view("\\n")
            '\r' => buffer.append_view("\\r")
            '\t' => buffer.append_view("\\t")
            '<' => {
                // Only `</` matters; a lone `<` is inert inside a script.
                if(i + 1u < len && data[i + 1u] == '/') {
                    // The `/` is KEPT: `i++` skips it in the input, so it has to
                    // be in the emitted text or the value silently loses it
                    // (`</script` would decode to `<script`).
                    buffer.append_view("\\u003C/")
                    i++
                } else {
                    buffer.append(c)
                }
            }
            default => {
                if((c as u8) < 0x20u8) {
                    buffer.append_view("\\u00")
                    append_page_writer_hex2(buffer, c as uint)
                } else {
                    buffer.append(c)
                }
            }
        }
        i++
    }
}

// The same for a CSS literal: a value that reaches a `<style>` element can close
// it with `</style>`, and a backslash would break the token.
func append_css_single_quoted_escaped(buffer : &mut std::string, data : *char, len : size_t) {
    var i : size_t = 0
    while(i < len) {
        const c = data[i]
        switch(c) {
            '\\' => buffer.append_view("\\\\")
            '<' => {
                if(i + 1u < len && data[i + 1u] == '/') {
                    buffer.append_view("\\3C ")
                    i++
                } else {
                    buffer.append(c)
                }
            }
            default => buffer.append(c)
        }
        i++
    }
}

public interface HtmlPageWriter {

    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue;

    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderHtmlAttrValue(page, getSsrAttributeValue(page))
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderJsAttrValue(page, getSsrAttributeValue(page))
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        renderCssAttrValue(page, getSsrAttributeValue(page))
    }

}

impl HtmlPageWriter for std::string_view {
    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue {
        return SsrAttributeValue.Text(SsrText { data : self.data(), size : self.size() });
    }
    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append_with_len(self.data(), self.size())
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        append_js_single_quoted_escaped(buffer, self.data(), self.size())
        buffer.append('\'');
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        append_css_single_quoted_escaped(buffer, self.data(), self.size())
        buffer.append('\'');
    }
}

impl HtmlPageWriter for std::string {
    func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue {
        return SsrAttributeValue.Text(SsrText { data : self.data(), size : self.size() });
    }
    func writeToPageHtml(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append_with_len(self.data(), self.size())
    }
    func writeToPageJs(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        append_js_single_quoted_escaped(buffer, self.data(), self.size())
        buffer.append('\'');
    }
    func writeToPageCss(&mut self, page : &mut HtmlPage, buffer : &mut std::string) {
        buffer.append('\'');
        append_css_single_quoted_escaped(buffer, self.data(), self.size())
        buffer.append('\'');
    }
}