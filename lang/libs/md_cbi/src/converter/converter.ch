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

func (converter : &mut MdConverter) convertChildren(children : &std::vector<*mut MdNode>) {
    var i = 0u;
    while(i < children.size()) {
        converter.convertMdNode(children.get(i));
        i++;
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
            converter.str.append_view("\">");
            converter.convertChildren(link.children);
            converter.str.append_view("</a>");
        }
        MdNodeKind.Image => {
            var img = node as *mut MdImage;
            converter.str.append_view("<img class=\"md-img\" src=\"");
            converter.str.append_view(img.url);
            converter.str.append_view("\" alt=\"");
            converter.str.append_view(img.alt);
            converter.str.append_view("\"/>");
        }
        MdNodeKind.InlineCode => {
            var code = node as *mut MdInlineCode;
            converter.str.append_view("<code class=\"md-code\">");
            converter.str.append_view(code.value);
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
            converter.str.append_view(cb.code);
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
                converter.str.append_view("<ol class=\"md-ol\">");
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
            converter.str.append_view("</li>");
        }
        MdNodeKind.Table => {
            var table = node as *mut MdTable;
            converter.str.append_view("<table class=\"md-table\">");
            converter.convertChildren(table.children);
            converter.str.append_view("</table>\n");
        }
        MdNodeKind.TableRow => {
            var row = node as *mut MdTableRow;
            converter.str.append_view("<tr class=\"md-tr\">");
            converter.convertChildren(row.children);
            converter.str.append_view("</tr>");
        }
        MdNodeKind.TableCell => {
            var cell = node as *mut MdTableCell;
            converter.str.append_view("<td class=\"md-td\">");
            converter.convertChildren(cell.children);
            converter.str.append_view("</td>");
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
    }
}

func (converter : &mut MdConverter) convertMdRoot(root : *mut MdRoot) {
    converter.convertMdNode(root as *mut MdNode);
    converter.put_chain_in();
}
