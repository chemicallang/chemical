public namespace md {

struct HtmlRenderer {
    var out : std::string

    func escape(&mut self, text : std::string_view) {
        // Defensive: ensure data is not null and size is reasonable
        const data = text.data()
        if(data == null) return
        const sz = text.size()
        var i = 0u;
        while(i < sz) {
            const c1 = (data[i] as uint) & 0xFF;
            if(c1 < 0x80) {
                const c = c1 as char;
                switch(c) {
                    '<' => { self.out.append_view("&lt;"); }
                    '>' => { self.out.append_view("&gt;"); }
                    '&' => { self.out.append_view("&amp;"); }
                    '"' => { self.out.append_view("&quot;"); }
                    default => { self.out.append(c); }
                }
                i++;
            } else {
                // For now: pass through UTF-8 bytes unchanged (common-case).
                self.out.append(data[i] as char);
                i++;
            }
        }
    }

    func render_children(&mut self, children : &std::vector<*mut MdNode>) {
        var i = 0u;
        while(i < children.size()) {
            self.render_node(children.get(i));
            i++;
        }
    }

    func render_node(&mut self, node : *mut MdNode) {
        if(node == null) return
        switch(node.kind) {
            MdNodeKind.Root => {
                var r = node as *mut MdRoot;
                self.render_children(r.children);
            }
            MdNodeKind.Header => {
                var h = node as *mut MdHeader;
                self.out.append_view("<h");
                self.out.append_integer(h.level as bigint);
                self.out.append_view("> ");
                self.render_children(h.children);
                self.out.append_view("</h");
                self.out.append_integer(h.level as bigint);
                self.out.append_view(">\n");
            }
            MdNodeKind.Paragraph => {
                var p = node as *mut MdParagraph;
                self.out.append_view("<p>");
                self.render_children(p.children);
                self.out.append_view("</p>\n");
            }
            MdNodeKind.Text => {
                var t = node as *mut MdText;
                if(t != null) {
                    self.escape(t.value);
                }
            }
            MdNodeKind.CodeBlock => {
                var cb = node as *mut MdCodeBlock;
                if(cb != null) {
                    self.out.append_view("<pre><code");
                    if(cb.language.size() > 0) {
                        self.out.append_view(" class=\"language-");
                        self.out.append_view(cb.language);
                        self.out.append_view("\"");
                    }
                    self.out.append_view(">");
                    self.escape(cb.code);
                    self.out.append_view("</code></pre>\n");
                }
            }
            default => {
                // Fallback: ignore node kinds not yet supported by runtime renderer.
            }
        }
    }
}

public func render_to_html(root : *mut MdRoot) : std::string {
    if(root == null) return std::string()
    var r = HtmlRenderer { out : std::string() }
    r.render_node(root as *mut MdNode)
    return std::replace(r.out, std::string())
}

}
