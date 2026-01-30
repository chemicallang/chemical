public namespace md {

public func render_inline_node(r : &mut HtmlRenderer, node : *mut MdNode) : bool {
    switch(node.kind) {
        MdNodeKind.Text => {
            var t = node as *mut MdText;
            if(t != null) r.escape(t.value);
            return true;
        }
        MdNodeKind.Bold => {
            var bold = node as *mut MdBold;
            if(bold != null) {
                r.out.append_view("<strong class=\"md-bold\">");
                render_children(r, bold.children);
                r.out.append_view("</strong>");
            }
            return true;
        }
        MdNodeKind.Italic => {
            var italic = node as *mut MdItalic;
            if(italic != null) {
                r.out.append_view("<em class=\"md-italic\">");
                render_children(r, italic.children);
                r.out.append_view("</em>");
            }
            return true;
        }
        MdNodeKind.Link => {
            var link = node as *mut MdLink;
            if(link != null) {
                r.out.append_view("<a class=\"md-link\" href=\"");
                r.escape(link.url);
                r.out.append_view("\">");
                render_children(r, link.children);
                r.out.append_view("</a>");
            }
            return true;
        }
        MdNodeKind.Image => {
            var img = node as *mut MdImage;
            if(img != null) {
                r.out.append_view("<img class=\"md-img\" src=\"");
                r.escape(img.url);
                r.out.append_view("\" alt=\"");
                r.escape(img.alt);
                r.out.append_view("\"/>");
            }
            return true;
        }
        MdNodeKind.InlineCode => {
            var code = node as *mut MdInlineCode;
            if(code != null) {
                r.out.append_view("<code class=\"md-code\">");
                r.escape(code.value);
                r.out.append_view("</code>");
            }
            return true;
        }
        MdNodeKind.Strikethrough => {
            var del = node as *mut MdStrikethrough;
            if(del != null) {
                r.out.append_view("<del class=\"md-del\">");
                render_children(r, del.children);
                r.out.append_view("</del>");
            }
            return true;
        }
        MdNodeKind.Mark => {
            var mark = node as *mut MdMark;
            if(mark != null) {
                r.out.append_view("<mark class=\"md-mark\">");
                render_children(r, mark.children);
                r.out.append_view("</mark>");
            }
            return true;
        }
        MdNodeKind.Insert => {
            var ins = node as *mut MdInsert;
            if(ins != null) {
                r.out.append_view("<ins class=\"md-ins\">");
                render_children(r, ins.children);
                r.out.append_view("</ins>");
            }
            return true;
        }
        MdNodeKind.Superscript => {
            var sup = node as *mut MdSuperscript;
            if(sup != null) {
                r.out.append_view("<sup class=\"md-sup\">");
                render_children(r, sup.children);
                r.out.append_view("</sup>");
            }
            return true;
        }
        MdNodeKind.Subscript => {
            var sub = node as *mut MdSubscript;
            if(sub != null) {
                r.out.append_view("<sub class=\"md-sub\">");
                render_children(r, sub.children);
                r.out.append_view("</sub>");
            }
            return true;
        }
        MdNodeKind.TaskCheckbox => {
            var checkbox = node as *mut MdTaskCheckbox;
            if(checkbox != null) {
                if(checkbox.checked) {
                    r.out.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled checked/>");
                } else {
                    r.out.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled/>");
                }
            }
            return true;
        }
        MdNodeKind.Footnote => {
            var footnote = node as *mut MdFootnote;
            if(footnote != null) {
                r.out.append_view("<sup class=\"md-footnote-ref\" id=\"fnref:");
                r.escape(footnote.id);
                r.out.append_view("\"><a href=\"#fn:");
                r.escape(footnote.id);
                r.out.append_view("\">");
                r.escape(footnote.id);
                r.out.append_view("</a></sup>");
            }
            return true;
        }
        MdNodeKind.Abbreviation => {
            var abbr = node as *mut MdAbbreviation;
            if(abbr != null) {
                if(abbr.title.size() > 0) {
                    r.out.append_view("<abbr class=\"md-abbr\" title=\" ");
                    r.escape(abbr.title);
                    r.out.append_view("\">");
                    r.escape(abbr.id);
                    r.out.append_view("</abbr>");
                } else {
                    r.escape(abbr.id);
                }
            }
            return true;
        }
        default => { return false; }
    }
}

}
