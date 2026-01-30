public namespace md {

public func render_block_node(r : &mut HtmlRenderer, node : *mut MdNode) : bool {
    switch(node.kind) {
        MdNodeKind.Root => {
            var root = node as *mut MdRoot;
            render_children(r, root.children);
            return true;
        }
        MdNodeKind.Header => {
            var h = node as *mut MdHeader;
            r.out.append_view("<h");
            r.out.append_integer(h.level as bigint);
            r.out.append_view(" class=\"md-hg md-h");
            r.out.append_integer(h.level as bigint);
            r.out.append_view("\">");
            render_children(r, h.children);
            r.out.append_view("</h");
            r.out.append_integer(h.level as bigint);
            r.out.append_view(">\n");
            return true;
        }
        MdNodeKind.Paragraph => {
            var p = node as *mut MdParagraph;
            r.out.append_view("<p class=\"md-p\">");
            render_children(r, p.children);
            r.out.append_view("</p>\n");
            return true;
        }
        MdNodeKind.CodeBlock => {
            var cb = node as *mut MdCodeBlock;
            if(cb != null) {
                r.out.append_view("<pre class=\"md-pre\"><code class=\"md-code-block");
                if(cb.language.size() > 0) {
                    r.out.append_view(" language-");
                    r.out.append_view(cb.language);
                }
                r.out.append_view("\">");
                r.escape(cb.code);
                r.out.append_view("</code></pre>\n");
            }
            return true;
        }
        MdNodeKind.List => {
            var list = node as *mut MdList;
            if(list != null) {
                if(list.ordered) {
                    r.out.append_view("<ol class=\"md-ol\">\n");
                } else {
                    r.out.append_view("<ul class=\"md-ul\">\n");
                }
                render_children(r, list.children);
                if(list.ordered) {
                    r.out.append_view("</ol>\n");
                } else {
                    r.out.append_view("</ul>\n");
                }
            }
            return true;
        }
        MdNodeKind.ListItem => {
            var item = node as *mut MdListItem;
            if(item != null) {
                r.out.append_view("<li class=\"md-li\">");
                render_children(r, item.children);
                r.out.append_view("</li>\n");
            }
            return true;
        }
        MdNodeKind.Table => {
            var table = node as *mut MdTable;
            if(table != null) {
                r.out.append_view("<table class=\"md-table\">\n");
                
                var i = 0u;
                var has_tbody = false;
                
                while(i < table.children.size()) {
                    var child = table.children.get(i);
                    if(child.kind == MdNodeKind.TableRow) {
                        var row = child as *mut MdTableRow;
                        
                        if(row.is_header) {
                            r.out.append_view("<thead class=\"md-thead\"><tr class=\"md-tr\">\n");
                            var j = 0u;
                            while(j < row.children.size()) {
                                var cell_child = row.children.get(j);
                                if(cell_child.kind == MdNodeKind.TableCell) {
                                    var cell = cell_child as *mut MdTableCell;
                                    r.out.append_view("<th class=\"md-th\"");
                                    
                                    if(j < table.alignments.size()) {
                                        const align = table.alignments.get(j);
                                        if(align == MdTableAlign.Left) {
                                            r.out.append_view(" style=\"text-align:left\"");
                                        } else if(align == MdTableAlign.Center) {
                                            r.out.append_view(" style=\"text-align:center\"");
                                        } else if(align == MdTableAlign.Right) {
                                            r.out.append_view(" style=\"text-align:right\"");
                                        }
                                    }
                                    
                                    r.out.append_view(">");
                                    render_children(r, cell.children);
                                    r.out.append_view("</th>\n");
                                }
                                j++;
                            }
                            r.out.append_view("</tr></thead>\n");
                        } else {
                            if(!has_tbody) {
                                r.out.append_view("<tbody class=\"md-tbody\"><tr class=\"md-tr\">\n");
                                has_tbody = true;
                            } else {
                                r.out.append_view("<tr class=\"md-tr\">\n");
                            }
                            
                            var j = 0u;
                            while(j < row.children.size()) {
                                var cell_child = row.children.get(j);
                                if(cell_child.kind == MdNodeKind.TableCell) {
                                    var cell = cell_child as *mut MdTableCell;
                                    r.out.append_view("<td class=\"md-td\"");
                                    
                                    if(j < table.alignments.size()) {
                                        const align = table.alignments.get(j);
                                        if(align == MdTableAlign.Left) {
                                            r.out.append_view(" style=\"text-align:left\"");
                                        } else if(align == MdTableAlign.Center) {
                                            r.out.append_view(" style=\"text-align:center\"");
                                        } else if(align == MdTableAlign.Right) {
                                            r.out.append_view(" style=\"text-align:right\"");
                                        }
                                    }
                                    
                                    r.out.append_view(">");
                                    render_children(r, cell.children);
                                    r.out.append_view("</td>\n");
                                }
                                j++;
                            }
                            r.out.append_view("</tr>\n");
                        }
                    }
                    i++;
                }
                
                if(has_tbody) {
                    r.out.append_view("</tbody>\n");
                }
                
                r.out.append_view("</table>\n");
            }
            return true;
        }
        MdNodeKind.Hr => {
            r.out.append_view("<hr class=\"md-hr\"/>\n");
            return true;
        }
        MdNodeKind.Blockquote => {
            var bq = node as *mut MdBlockquote;
            if(bq != null) {
                r.out.append_view("<blockquote class=\"md-blockquote\">");
                render_children(r, bq.children);
                r.out.append_view("</blockquote>\n");
            }
            return true;
        }
        MdNodeKind.CustomContainer => {
            var container = node as *mut MdCustomContainer;
            if(container != null) {
                r.out.append_view("<div class=\"md-container md-");
                r.out.append_view(container.type);
                r.out.append_view("\">");
                render_children(r, container.children);
                r.out.append_view("</div>\n");
            }
            return true;
        }
        MdNodeKind.DefinitionList => {
            var dl = node as *mut MdDefinitionList;
            if(dl != null) {
                r.out.append_view("<dl class=\"md-dl\">");
                render_children(r, dl.children);
                r.out.append_view("</dl>\n");
            }
            return true;
        }
        MdNodeKind.DefinitionTerm => {
            var dt = node as *mut MdDefinitionTerm;
            if(dt != null) {
                r.out.append_view("<dt class=\"md-dt\">");
                render_children(r, dt.children);
                r.out.append_view("</dt>\n");
            }
            return true;
        }
        MdNodeKind.DefinitionData => {
            var dd = node as *mut MdDefinitionData;
            if(dd != null) {
                r.out.append_view("<dd class=\"md-dd\">");
                render_children(r, dd.children);
                r.out.append_view("</dd>\n");
            }
            return true;
        }
        MdNodeKind.FootnoteDef => {
            var footnote_def = node as *mut MdFootnoteDef;
            if(footnote_def != null) {
                r.out.append_view("<div class=\"md-footnote-def\" id=\"fn:");
                r.escape(footnote_def.id);
                r.out.append_view("\"><span class=\"md-footnote-id\">");
                r.escape(footnote_def.id);
                r.out.append_view(": </span>");
                render_children(r, footnote_def.children);
                r.out.append_view("</div>\n");
            }
            return true;
        }
        default => { return false; }
    }
}

}
