struct MdConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
}

func (converter : &mut MdConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut MdConverter) make_html_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_html"), converter.support.appendHtmlFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut MdConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(converter.str.to_view()), location)
    const size = converter.str.size()
    converter.str.clear();
    
    if(converter.support.pageNode != null) {
        const call = converter.make_html_value_call(value, size);
        converter.vec.push(call as *mut ASTNode);
    }
}

func (converter : &mut MdConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    if(converter.support.pageNode == null) return;
    
    const type = value.getType()
    var call : *mut FunctionCallNode = null;
    
    switch(type.getKind()) {
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            if(intN.get_intn_type_kind() <= IntNTypeKind.Int128) {
                call = converter.make_value_call_with(value, std::string_view("append_html_integer"), converter.support.appendHtmlIntFn)
            } else {
                call = converter.make_value_call_with(value, std::string_view("append_html_uinteger"), converter.support.appendHtmlUIntFn)
            }
        }
        BaseTypeKind.Float => {
            call = converter.make_value_call_with(value, std::string_view("append_html_float"), converter.support.appendHtmlFloatFn)
        }
        BaseTypeKind.Double => {
            call = converter.make_value_call_with(value, std::string_view("append_html_double"), converter.support.appendHtmlDoubleFn)
        }
        default => {
            call = converter.make_value_call_with(value, std::string_view("append_html_char_ptr"), converter.support.appendHtmlCharPtrFn)
        }
    }
    
    if(call != null) {
        converter.vec.push(call as *mut ASTNode)
    }
}

func (converter : &mut MdConverter) escapeHtml(text : std::string_view) {
    var i = 0u;
    while(i < text.size()) {
        const c = text.data()[i];
        switch(c) {
            '<' => { converter.str.append_view("&lt;"); }
            '>' => { converter.str.append_view("&gt;"); }
            '&' => { converter.str.append_view("&amp;"); }
            '"' => { converter.str.append_view("&quot;"); }
            default => { converter.str.append(c); }
        }
        i++;
    }
}

func (converter : &mut MdConverter) convertChildren(children : &std::vector<*mut MdNode>) {
    var i = 0u;
    while(i < children.size()) {
        converter.convertMdNode(children.get(i));
        i++;
    }
}

func (converter : &mut MdConverter) getAlignStyle(align : MdTableAlign) : std::string_view {
    switch(align) {
        MdTableAlign.Left => { return std::string_view(" style=\"text-align:left\""); }
        MdTableAlign.Center => { return std::string_view(" style=\"text-align:center\""); }
        MdTableAlign.Right => { return std::string_view(" style=\"text-align:right\""); }
        default => { return std::string_view(""); }
    }
}

func (converter : &mut MdConverter) convertMdNode(node : *mut MdNode) {
    switch(node.kind) {
        MdNodeKind.Root => {
            var root = node as *mut MdRoot;
            converter.convertChildren(root.children);
        }
        MdNodeKind.Header => {
            var header = node as *mut MdHeader;
            converter.str.append_view("<h");
            converter.str.append_integer(header.level as bigint);
            converter.str.append_view(" class=\"md-hg md-h");
            converter.str.append_integer(header.level as bigint);
            converter.str.append_view("\">");
            converter.convertChildren(header.children);
            converter.str.append_view("</h");
            converter.str.append_integer(header.level as bigint);
            converter.str.append_view(">\n");
        }
        MdNodeKind.Paragraph => {
            var para = node as *mut MdParagraph;
            converter.str.append_view("<p class=\"md-p\">");
            converter.convertChildren(para.children);
            converter.str.append_view("</p>\n");
        }
        MdNodeKind.Bold => {
            var bold = node as *mut MdBold;
            converter.str.append_view("<strong class=\"md-bold\">");
            converter.convertChildren(bold.children);
            converter.str.append_view("</strong>");
        }
        MdNodeKind.Italic => {
            var italic = node as *mut MdItalic;
            converter.str.append_view("<em class=\"md-italic\">");
            converter.convertChildren(italic.children);
            converter.str.append_view("</em>");
        }
        MdNodeKind.Strikethrough => {
            var strike = node as *mut MdStrikethrough;
            converter.str.append_view("<del class=\"md-del\">");
            converter.convertChildren(strike.children);
            converter.str.append_view("</del>");
        }
        MdNodeKind.Link => {
            var link = node as *mut MdLink;
            converter.str.append_view("<a class=\"md-link\" href=\"");
            converter.str.append_view(link.url);
            converter.str.append_view("\"");
            if(link.title.size() > 0) {
                converter.str.append_view(" title=\"");
                converter.str.append_view(link.title);
                converter.str.append_view("\"");
            }
            converter.str.append_view(">");
            converter.convertChildren(link.children);
            converter.str.append_view("</a>");
        }
        MdNodeKind.AutoLink => {
            var autolink = node as *mut MdAutoLink;
            converter.str.append_view("<a class=\"md-link md-autolink\" href=\"");
            converter.str.append_view(autolink.url);
            converter.str.append_view("\">");
            converter.str.append_view(autolink.url);
            converter.str.append_view("</a>");
        }
        MdNodeKind.Image => {
            var img = node as *mut MdImage;
            converter.str.append_view("<img class=\"md-img\" src=\"");
            converter.str.append_view(img.url);
            converter.str.append_view("\" alt=\"");
            converter.str.append_view(img.alt);
            converter.str.append_view("\"");
            if(img.title.size() > 0) {
                converter.str.append_view(" title=\"");
                converter.str.append_view(img.title);
                converter.str.append_view("\"");
            }
            converter.str.append_view("/>");
        }
        MdNodeKind.InlineCode => {
            var code = node as *mut MdInlineCode;
            converter.str.append_view("<code class=\"md-code\">");
            converter.escapeHtml(code.value);
            converter.str.append_view("</code>");
        }
        MdNodeKind.CodeBlock => {
            var cb = node as *mut MdCodeBlock;
            converter.str.append_view("<pre class=\"md-pre\"><code class=\"md-code-block");
            if(cb.language.size() > 0) {
                converter.str.append_view(" language-");
                converter.str.append_view(cb.language);
            }
            converter.str.append_view("\">");
            converter.escapeHtml(cb.code);
            converter.str.append_view("</code></pre>\n");
        }
        MdNodeKind.Blockquote => {
            var bq = node as *mut MdBlockquote;
            converter.str.append_view("<blockquote class=\"md-blockquote\">");
            converter.convertChildren(bq.children);
            converter.str.append_view("</blockquote>\n");
        }
        MdNodeKind.Hr => {
            converter.str.append_view("<hr class=\"md-hr\"/>\n");
        }
        MdNodeKind.List => {
            var list = node as *mut MdList;
            if(list.ordered) {
                converter.str.append_view("<ol class=\"md-ol\"");
                if(list.start != 1) {
                    converter.str.append_view(" start=\"");
                    converter.str.append_integer(list.start as bigint);
                    converter.str.append_view("\"");
                }
                converter.str.append_view(">");
            } else {
                converter.str.append_view("<ul class=\"md-ul\">");
            }
            converter.convertChildren(list.children);
            if(list.ordered) {
                converter.str.append_view("</ol>\n");
            } else {
                converter.str.append_view("</ul>\n");
            }
        }
        MdNodeKind.ListItem => {
            var item = node as *mut MdListItem;
            converter.str.append_view("<li class=\"md-li\">");
            converter.convertChildren(item.children);
            converter.str.append_view("</li>\n");
        }
        MdNodeKind.Table => {
            var table = node as *mut MdTable;
            converter.str.append_view("<table class=\"md-table\">\n");
            
            var row_idx = 0u;
            while(row_idx < table.children.size()) {
                const row_node = table.children.get(row_idx);
                const row = row_node as *mut MdTableRow;
                
                if(row.is_header) {
                    converter.str.append_view("<thead class=\"md-thead\">\n");
                    converter.str.append_view("<tr class=\"md-tr\">");
                    
                    var col_idx = 0u;
                    while(col_idx < row.children.size()) {
                        const cell = row.children.get(col_idx) as *mut MdTableCell;
                        converter.str.append_view("<th class=\"md-th\"");
                        if(col_idx < table.alignments.size()) {
                            converter.str.append_view(converter.getAlignStyle(table.alignments.get(col_idx)));
                        }
                        converter.str.append_view(">");
                        converter.convertChildren(cell.children);
                        converter.str.append_view("</th>");
                        col_idx++;
                    }
                    
                    converter.str.append_view("</tr>\n");
                    converter.str.append_view("</thead>\n<tbody class=\"md-tbody\">\n");
                } else {
                    converter.str.append_view("<tr class=\"md-tr\">");
                    
                    var col_idx = 0u;
                    while(col_idx < row.children.size()) {
                        const cell = row.children.get(col_idx) as *mut MdTableCell;
                        converter.str.append_view("<td class=\"md-td\"");
                        if(col_idx < table.alignments.size()) {
                            converter.str.append_view(converter.getAlignStyle(table.alignments.get(col_idx)));
                        }
                        converter.str.append_view(">");
                        converter.convertChildren(cell.children);
                        converter.str.append_view("</td>");
                        col_idx++;
                    }
                    
                    converter.str.append_view("</tr>\n");
                }
                
                row_idx++;
            }
            
            converter.str.append_view("</tbody>\n</table>\n");
        }
        MdNodeKind.TableRow => {
            // Handled in Table conversion
        }
        MdNodeKind.TableCell => {
            // Handled in Table conversion
        }
        MdNodeKind.Text => {
            var text = node as *mut MdText;
            converter.str.append_view(text.value);
        }
        MdNodeKind.Interpolation => {
            var interp = node as *mut MdInterpolation;
            converter.put_chain_in();
            converter.put_wrapped_chemical_value_in(interp.value);
        }
        MdNodeKind.Superscript => {
            var sup = node as *mut MdSuperscript;
            converter.str.append_view("<sup class=\"md-sup\">");
            converter.convertChildren(sup.children);
            converter.str.append_view("</sup>");
        }
        MdNodeKind.Subscript => {
            var sub = node as *mut MdSubscript;
            converter.str.append_view("<sub class=\"md-sub\">");
            converter.convertChildren(sub.children);
            converter.str.append_view("</sub>");
        }
        MdNodeKind.Insert => {
            var ins = node as *mut MdInsert;
            converter.str.append_view("<ins class=\"md-ins\">");
            converter.convertChildren(ins.children);
            converter.str.append_view("</ins>");
        }
        MdNodeKind.Mark => {
            var mark = node as *mut MdMark;
            converter.str.append_view("<mark class=\"md-mark\">");
            converter.convertChildren(mark.children);
            converter.str.append_view("</mark>");
        }
        MdNodeKind.Footnote => {
            var fn = node as *mut MdFootnote;
            converter.str.append_view("<sup class=\"md-footnote-ref\" id=\"fnref:");
            converter.str.append_view(fn.id);
            converter.str.append_view("\"><a href=\"#fn:");
            converter.str.append_view(fn.id);
            converter.str.append_view("\">");
            converter.str.append_view(fn.id);
            converter.str.append_view("</a></sup>");
        }
        MdNodeKind.FootnoteDef => {
            var fd = node as *mut MdFootnoteDef;
            converter.str.append_view("<div class=\"md-footnote-def\" id=\"fn:");
            converter.str.append_view(fd.id);
            converter.str.append_view("\"><span class=\"md-footnote-id\">");
            converter.str.append_view(fd.id);
            converter.str.append_view(": </span>");
            converter.convertChildren(fd.children);
            converter.str.append_view("</div>\n");
        }
        MdNodeKind.DefinitionList => {
            var dl = node as *mut MdDefinitionList;
            converter.str.append_view("<dl class=\"md-dl\">\n");
            converter.convertChildren(dl.children);
            converter.str.append_view("</dl>\n");
        }
        MdNodeKind.DefinitionTerm => {
            var dt = node as *mut MdDefinitionTerm;
            converter.str.append_view("<dt class=\"md-dt\">");
            converter.convertChildren(dt.children);
            converter.str.append_view("</dt>\n");
        }
        MdNodeKind.DefinitionData => {
            var dd = node as *mut MdDefinitionData;
            converter.str.append_view("<dd class=\"md-dd\">");
            converter.convertChildren(dd.children);
            converter.str.append_view("</dd>\n");
        }
        MdNodeKind.Abbreviation => {
            var abb = node as *mut MdAbbreviation;
            converter.str.append_view("<abbr title=\"");
            converter.str.append_view(abb.title);
            converter.str.append_view("\">");
            converter.str.append_view(abb.id);
            converter.str.append_view("</abbr>");
        }
        MdNodeKind.CustomContainer => {
            var cc = node as *mut MdCustomContainer;
            converter.str.append_view("<div class=\"md-container md-");
            converter.str.append_view(cc.type);
            converter.str.append_view("\">\n");
            converter.convertChildren(cc.children);
            converter.str.append_view("</div>\n");
        }
    }
}

func (converter : &mut MdConverter) convertMdRoot(root : *mut MdRoot) {
    converter.convertMdNode(root as *mut MdNode);
    converter.put_chain_in();
}
