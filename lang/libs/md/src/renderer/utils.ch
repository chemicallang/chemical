public namespace md {

public struct HtmlRenderer {
    var out : std::string

    func escape(&mut self, text : std::string_view) {
        const data = text.data()
        if(data == null) return
        const sz = text.size()
        var i = 0u;
        while(i < sz) {
            const c = data[i];
            if(c as uint >= 0x80) {
                self.out.append(c);
                i++;
                while(i < sz && (data[i] as uint >= 0x80 && data[i] as uint < 0xC0)) {
                    self.out.append(data[i]);
                    i++;
                }
            } else if(c == '<') {
                self.out.append_view("&lt;");
                i++;
            } else if(c == '>') {
                self.out.append_view("&gt;");
                i++;
            } else if(c == '&') {
                self.out.append_view("&amp;");
                i++;
            } else if(c == '"') {
                self.out.append_view("&quot;");
                i++;
            } else {
                self.out.append(c);
                i++;
            }
        }
    }
}

public func render_children(r : &mut HtmlRenderer, children : &std::vector<*mut MdNode>) {
    var i = 0u;
    while(i < children.size()) {
        render_node(r, children.get(i));
        i++;
    }
}

// Forward declare render_node since it's recursive and circular
// In Chemical, effectively all functions in namespace are visible.
// We will define render_node in html.ch or a central place, 
// OR we can define it in utils.ch if we include block.ch and inline.ch logic?
// Ideally `render_node` dispatches.
// For now, I'll put a placeholder or rely on `block.ch` / `inline.ch` to verify visibility.

}
