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
            MdNodeKind.List => {
                var list = node as *mut MdList;
                if(list != null) {
                    if(list.ordered) {
                        self.out.append_view("<ol>\n");
                    } else {
                        self.out.append_view("<ul>\n");
                    }
                    self.render_children(list.children);
                    if(list.ordered) {
                        self.out.append_view("</ol>\n");
                    } else {
                        self.out.append_view("</ul>\n");
                    }
                }
            }
            MdNodeKind.ListItem => {
                var item = node as *mut MdListItem;
                if(item != null) {
                    self.out.append_view("<li>");
                    self.render_children(item.children);
                    self.out.append_view("</li>\n");
                }
            }
            MdNodeKind.Table => {
                var table = node as *mut MdTable;
                if(table != null) {
                    self.out.append_view("<table>\n");
                    self.render_children(table.children);
                    self.out.append_view("</table>\n");
                }
            }
            MdNodeKind.TableRow => {
                var row = node as *mut MdTableRow;
                if(row != null) {
                    if(row.is_header) {
                        self.out.append_view("<thead><tr>\n");
                    } else {
                        self.out.append_view("<tr>\n");
                    }
                    self.render_children(row.children);
                    if(row.is_header) {
                        self.out.append_view("</tr></thead>\n");
                    } else {
                        self.out.append_view("</tr>\n");
                    }
                }
            }
            MdNodeKind.TableCell => {
                var cell = node as *mut MdTableCell;
                if(cell != null) {
                    self.out.append_view("<td>");
                    self.render_children(cell.children);
                    self.out.append_view("</td>");
                }
            }
            MdNodeKind.Hr => {
                self.out.append_view("<hr/>\n");
            }
            MdNodeKind.Bold => {
                var bold = node as *mut MdBold;
                if(bold != null) {
                    self.out.append_view("<strong>");
                    self.render_children(bold.children);
                    self.out.append_view("</strong>");
                }
            }
            MdNodeKind.Italic => {
                var italic = node as *mut MdItalic;
                if(italic != null) {
                    self.out.append_view("<em>");
                    self.render_children(italic.children);
                    self.out.append_view("</em>");
                }
            }
            MdNodeKind.Link => {
                var link = node as *mut MdLink;
                if(link != null) {
                    self.out.append_view("<a href=\"");
                    self.escape(link.url);
                    self.out.append_view("\">");
                    self.render_children(link.children);
                    self.out.append_view("</a>");
                }
            }
            MdNodeKind.Image => {
                var img = node as *mut MdImage;
                if(img != null) {
                    self.out.append_view("<img src=\"");
                    self.escape(img.url);
                    self.out.append_view("\" alt=\"");
                    self.escape(img.alt);
                    self.out.append_view("\"/>");
                }
            }
            MdNodeKind.InlineCode => {
                var code = node as *mut MdInlineCode;
                if(code != null) {
                    self.out.append_view("<code>");
                    self.escape(code.value);
                    self.out.append_view("</code>");
                }
            }
            MdNodeKind.Strikethrough => {
                var del = node as *mut MdStrikethrough;
                if(del != null) {
                    self.out.append_view("<del>");
                    self.render_children(del.children);
                    self.out.append_view("</del>");
                }
            }
            MdNodeKind.Mark => {
                var mark = node as *mut MdMark;
                if(mark != null) {
                    self.out.append_view("<mark>");
                    self.render_children(mark.children);
                    self.out.append_view("</mark>");
                }
            }
            MdNodeKind.Insert => {
                var ins = node as *mut MdInsert;
                if(ins != null) {
                    self.out.append_view("<ins>");
                    self.render_children(ins.children);
                    self.out.append_view("</ins>");
                }
            }
            MdNodeKind.Superscript => {
                var sup = node as *mut MdSuperscript;
                if(sup != null) {
                    self.out.append_view("<sup>");
                    self.render_children(sup.children);
                    self.out.append_view("</sup>");
                }
            }
            MdNodeKind.Subscript => {
                var sub = node as *mut MdSubscript;
                if(sub != null) {
                    self.out.append_view("<sub>");
                    self.render_children(sub.children);
                    self.out.append_view("</sub>");
                }
            }
            MdNodeKind.Blockquote => {
                var bq = node as *mut MdBlockquote;
                if(bq != null) {
                    self.out.append_view("<blockquote>");
                    self.render_children(bq.children);
                    self.out.append_view("</blockquote>\n");
                }
            }
            MdNodeKind.CustomContainer => {
                var container = node as *mut MdCustomContainer;
                if(container != null) {
                    self.out.append_view("<div class=\"md-container md-");
                    self.out.append_view(container.type);
                    self.out.append_view("\">");
                    self.render_children(container.children);
                    self.out.append_view("</div>\n");
                }
            }
            MdNodeKind.TaskCheckbox => {
                var checkbox = node as *mut MdTaskCheckbox;
                if(checkbox != null) {
                    self.out.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled");
                    if(checkbox.checked) {
                        self.out.append_view(" checked");
                    }
                    self.out.append_view("/>");
                }
            }
            MdNodeKind.DefinitionList => {
                var dl = node as *mut MdDefinitionList;
                if(dl != null) {
                    self.out.append_view("<dl class=\"md-dl\">\n");
                    self.render_children(dl.children);
                    self.out.append_view("</dl>\n");
                }
            }
            MdNodeKind.DefinitionTerm => {
                var dt = node as *mut MdDefinitionTerm;
                if(dt != null) {
                    self.out.append_view("<dt class=\"md-dt\">");
                    self.render_children(dt.children);
                    self.out.append_view("</dt>\n");
                }
            }
            MdNodeKind.DefinitionData => {
                var dd = node as *mut MdDefinitionData;
                if(dd != null) {
                    self.out.append_view("<dd class=\"md-dd\">");
                    self.render_children(dd.children);
                    self.out.append_view("</dd>\n");
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
