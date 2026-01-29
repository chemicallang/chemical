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
            const c = data[i];
            // Handle UTF-8 multi-byte sequences - pass through unchanged
            if(c as uint >= 0x80) {
                // This is the start of a UTF-8 sequence, pass through all bytes
                self.out.append(c);
                i++;
                // Pass through continuation bytes
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
                self.out.append_view(" class=\"md-hg md-h");
                self.out.append_integer(h.level as bigint);
                self.out.append_view("\">");
                self.render_children(h.children);
                self.out.append_view("</h");
                self.out.append_integer(h.level as bigint);
                self.out.append_view(">\n");
            }
            MdNodeKind.Paragraph => {
                var p = node as *mut MdParagraph;
                self.out.append_view("<p class=\"md-p\">");
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
                    self.out.append_view("<pre class=\"md-pre\"><code class=\"md-code-block");
                    if(cb.language.size() > 0) {
                        self.out.append_view(" language-");
                        self.out.append_view(cb.language);
                    }
                    self.out.append_view("\">");
                    self.escape(cb.code);
                    self.out.append_view("</code></pre>\n");
                }
            }
            MdNodeKind.List => {
                var list = node as *mut MdList;
                if(list != null) {
                    if(list.ordered) {
                        self.out.append_view("<ol class=\"md-ol\">\n");
                    } else {
                        self.out.append_view("<ul class=\"md-ul\">\n");
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
                    self.out.append_view("<li class=\"md-li\">");
                    self.render_children(item.children);
                    self.out.append_view("</li>\n");
                }
            }
            MdNodeKind.Table => {
                var table = node as *mut MdTable;
                if(table != null) {
                    self.out.append_view("<table class=\"md-table\">\n");
                    
                    var i = 0u;
                    var has_tbody = false;
                    
                    while(i < table.children.size()) {
                        var child = table.children.get(i);
                        if(child.kind == MdNodeKind.TableRow) {
                            var row = child as *mut MdTableRow;
                            
                            if(row.is_header) {
                                self.out.append_view("<thead class=\"md-thead\"><tr class=\"md-tr\">\n");
                                var j = 0u;
                                while(j < row.children.size()) {
                                    var cell_child = row.children.get(j);
                                    if(cell_child.kind == MdNodeKind.TableCell) {
                                        var cell = cell_child as *mut MdTableCell;
                                        self.out.append_view("<th class=\"md-th\">");
                                        
                                        // Add alignment style if available
                                        if(j < table.alignments.size()) {
                                            const align = table.alignments.get(j);
                                            if(align == MdTableAlign.Left) {
                                                self.out.append_view(" style=\"text-align:left\"");
                                            } else if(align == MdTableAlign.Center) {
                                                self.out.append_view(" style=\"text-align:center\"");
                                            } else if(align == MdTableAlign.Right) {
                                                self.out.append_view(" style=\"text-align:right\"");
                                            }
                                        }
                                        
                                        self.out.append_view(">");
                                        self.render_children(cell.children);
                                        self.out.append_view("</th>\n");
                                    }
                                    j++;
                                }
                                self.out.append_view("</tr></thead>\n");
                            } else {
                                if(!has_tbody) {
                                    self.out.append_view("<tbody class=\"md-tbody\"><tr class=\"md-tr\">\n");
                                    has_tbody = true;
                                } else {
                                    self.out.append_view("<tr class=\"md-tr\">\n");
                                }
                                
                                var j = 0u;
                                while(j < row.children.size()) {
                                    var cell_child = row.children.get(j);
                                    if(cell_child.kind == MdNodeKind.TableCell) {
                                        var cell = cell_child as *mut MdTableCell;
                                        self.out.append_view("<td class=\"md-td\">");
                                        
                                        // Add alignment style if available
                                        if(j < table.alignments.size()) {
                                            const align = table.alignments.get(j);
                                            if(align == MdTableAlign.Left) {
                                                self.out.append_view(" style=\"text-align:left\"");
                                            } else if(align == MdTableAlign.Center) {
                                                self.out.append_view(" style=\"text-align:center\"");
                                            } else if(align == MdTableAlign.Right) {
                                                self.out.append_view(" style=\"text-align:right\"");
                                            }
                                        }
                                        
                                        self.out.append_view(">");
                                        self.render_children(cell.children);
                                        self.out.append_view("</td>\n");
                                    }
                                    j++;
                                }
                                self.out.append_view("</tr>\n");
                            }
                        }
                        i++;
                    }
                    
                    if(has_tbody) {
                        self.out.append_view("</tbody>\n");
                    }
                    
                    self.out.append_view("</table>\n");
                }
            }
            MdNodeKind.TableCell => {
                var cell = node as *mut MdTableCell;
                if(cell != null) {
                    self.out.append_view("<td class=\"md-td\">");
                    self.render_children(cell.children);
                    self.out.append_view("</td>");
                }
            }
            MdNodeKind.Hr => {
                self.out.append_view("<hr class=\"md-hr\"/>\n");
            }
            MdNodeKind.Bold => {
                var bold = node as *mut MdBold;
                if(bold != null) {
                    self.out.append_view("<strong class=\"md-bold\">");
                    self.render_children(bold.children);
                    self.out.append_view("</strong>");
                }
            }
            MdNodeKind.Italic => {
                var italic = node as *mut MdItalic;
                if(italic != null) {
                    self.out.append_view("<em class=\"md-italic\">");
                    self.render_children(italic.children);
                    self.out.append_view("</em>");
                }
            }
            MdNodeKind.Link => {
                var link = node as *mut MdLink;
                if(link != null) {
                    self.out.append_view("<a class=\"md-link\" href=\"");
                    self.escape(link.url);
                    self.out.append_view("\">");
                    self.render_children(link.children);
                    self.out.append_view("</a>");
                }
            }
            MdNodeKind.Image => {
                var img = node as *mut MdImage;
                if(img != null) {
                    self.out.append_view("<img class=\"md-img\" src=\"");
                    self.escape(img.url);
                    self.out.append_view("\" alt=\"");
                    self.escape(img.alt);
                    self.out.append_view("\"/>");
                }
            }
            MdNodeKind.InlineCode => {
                var code = node as *mut MdInlineCode;
                if(code != null) {
                    self.out.append_view("<code class=\"md-code\">");
                    self.escape(code.value);
                    self.out.append_view("</code>");
                }
            }
            MdNodeKind.Strikethrough => {
                var del = node as *mut MdStrikethrough;
                if(del != null) {
                    self.out.append_view("<del class=\"md-del\">");
                    self.render_children(del.children);
                    self.out.append_view("</del>");
                }
            }
            MdNodeKind.Mark => {
                var mark = node as *mut MdMark;
                if(mark != null) {
                    self.out.append_view("<mark class=\"md-mark\">");
                    self.render_children(mark.children);
                    self.out.append_view("</mark>");
                }
            }
            MdNodeKind.Insert => {
                var ins = node as *mut MdInsert;
                if(ins != null) {
                    self.out.append_view("<ins class=\"md-ins\">");
                    self.render_children(ins.children);
                    self.out.append_view("</ins>");
                }
            }
            MdNodeKind.Superscript => {
                var sup = node as *mut MdSuperscript;
                if(sup != null) {
                    self.out.append_view("<sup class=\"md-sup\">");
                    self.render_children(sup.children);
                    self.out.append_view("</sup>");
                }
            }
            MdNodeKind.Subscript => {
                var sub = node as *mut MdSubscript;
                if(sub != null) {
                    self.out.append_view("<sub class=\"md-sub\">");
                    self.render_children(sub.children);
                    self.out.append_view("</sub>");
                }
            }
            MdNodeKind.Blockquote => {
                var bq = node as *mut MdBlockquote;
                if(bq != null) {
                    self.out.append_view("<blockquote class=\"md-blockquote\">");
                    self.render_children(bq.children);
                    self.out.append_view("</blockquote>\n");
                }
            }
            MdNodeKind.TaskCheckbox => {
                var checkbox = node as *mut MdTaskCheckbox;
                if(checkbox != null) {
                    if(checkbox.checked) {
                        self.out.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled checked/>");
                    } else {
                        self.out.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled/>");
                    }
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
            MdNodeKind.DefinitionList => {
                var dl = node as *mut MdDefinitionList;
                if(dl != null) {
                    self.out.append_view("<dl class=\"md-dl\">");
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
            MdNodeKind.Footnote => {
                var footnote = node as *mut MdFootnote;
                if(footnote != null) {
                    self.out.append_view("<sup class=\"md-footnote-ref\" id=\"fnref:");
                    self.escape(footnote.id);
                    self.out.append_view("\"><a href=\"#fn:");
                    self.escape(footnote.id);
                    self.out.append_view("\">");
                    self.escape(footnote.id);
                    self.out.append_view("</a></sup>");
                }
            }
            MdNodeKind.FootnoteDef => {
                var footnote_def = node as *mut MdFootnoteDef;
                if(footnote_def != null) {
                    self.out.append_view("<div class=\"md-footnote-def\" id=\"fn:");
                    self.escape(footnote_def.id);
                    self.out.append_view("\"><span class=\"md-footnote-id\">");
                    self.escape(footnote_def.id);
                    self.out.append_view(": </span>");
                    self.render_children(footnote_def.children);
                    self.out.append_view("</div>\n");
                }
            }
            MdNodeKind.Abbreviation => {
                var abbr = node as *mut MdAbbreviation;
                if(abbr != null) {
                    if(abbr.title.size() > 0) {
                        // This is an abbreviation definition - render as abbr tag
                        self.out.append_view("<abbr title=\" ");
                        self.escape(abbr.title);
                        self.out.append_view("\">");
                        self.escape(abbr.id);
                        self.out.append_view("</abbr>");
                    } else {
                        // This is an abbreviation reference - render as abbr tag without title
                        // In markdown, abbreviation references should be rendered as abbr tags
                        // but the title should come from the definition. For now, render as text.
                        self.escape(abbr.id);
                    }
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
