public namespace md {

struct MdConverter {
    var str : std::string
    var highlighter : (lang : std::string_view, code : std::string_view) => std::string
    var link_rewriter : (url : std::string_view) => std::string
}

// ─── MdEmitter implementation ───────────────────────────────────────────────

impl MdEmitter for MdConverter {
    func emit_text(&mut self, text : &std::string_view) { self.str.append_view(text) }
    func emit_char(&mut self, c : char) { self.str.append(c) }
    func emit_integer(&mut self, v : bigint) { self.str.append_integer(v) }
    func flush(&mut self) {}
    func emit_interpolation(&mut self, value : *mut Value) {}
}

// ─── Public API ─────────────────────────────────────────────────────────────

func render_to_html(root : *mut MdRoot, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) : std::string {
    var converter = MdConverter { str : std::string(), highlighter : highlighter, link_rewriter : link_rewriter }
    converter.convertMdRoot(root)
    return std::replace(&mut converter.str, std::string())
}

func (converter : &mut MdConverter) convertMdRoot(root : *mut MdRoot) {
    if(root != null) {
        md_convert_md_node(&mut converter.str, root as *mut MdNode, converter as *mut MdEmitter, converter.highlighter, converter.link_rewriter)
    }
}

} // namespace md
