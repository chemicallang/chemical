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
    
    // Only if we have a page node, otherwise we are just building a string?
    // In md_replacementValue, we set calculated value.
    // If we have page node, we also push function calls.
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

func (converter : &mut MdConverter) convertMdNode(node : *mut MdNode) {
    switch(node.kind) {
        MdNodeKind.Root => {
             var root = node as *mut MdRoot
             var i = 0u
             while(i < root.children.size()) {
                 converter.convertMdNode(root.children.get(i))
                 i++
             }
        }
        MdNodeKind.Header => {
            var header = node as *mut MdHeader
            converter.str.append_view("<h")
            converter.str.append_integer(header.level as bigint)
            converter.str.append_view(">")
            
            var i = 0u
            while(i < header.children.size()) {
                converter.convertMdNode(header.children.get(i))
                i++
            }
            
            converter.str.append_view("</h")
            converter.str.append_integer(header.level as bigint)
            converter.str.append_view(">")
            converter.str.append_view("\n")
        }
        MdNodeKind.Paragraph => {
            var para = node as *mut MdParagraph
            converter.str.append_view("<p>")
            var i = 0u
            while(i < para.children.size()) {
                converter.convertMdNode(para.children.get(i))
                i++
            }
            converter.str.append_view("</p>\n")
        }
        MdNodeKind.Bold => {
            var bold = node as *mut MdBold
            converter.str.append_view("<strong>")
            var i = 0u
            while(i < bold.children.size()) {
                converter.convertMdNode(bold.children.get(i))
                i++
            }
            converter.str.append_view("</strong>")
        }
        MdNodeKind.Italic => {
            var italic = node as *mut MdItalic
            converter.str.append_view("<em>")
            var i = 0u
            while(i < italic.children.size()) {
                converter.convertMdNode(italic.children.get(i))
                i++
            }
            converter.str.append_view("</em>")
        }
        MdNodeKind.Link => {
            var link = node as *mut MdLink
            converter.str.append_view("<a href=\"")
            converter.str.append_view(link.url)
            converter.str.append_view("\">")
            var i = 0u
            while(i < link.children.size()) {
                converter.convertMdNode(link.children.get(i))
                i++
            }
            converter.str.append_view("</a>")
        }
        MdNodeKind.Image => {
            var img = node as *mut MdImage
            converter.str.append_view("<img src=\"")
            converter.str.append_view(img.url)
            converter.str.append_view("\" alt=\"")
            converter.str.append_view(img.alt)
            converter.str.append_view("\" />")
        }
        MdNodeKind.InlineCode => {
            var code = node as *mut MdInlineCode
            converter.str.append_view("<code>")
            converter.str.append_view(code.value)
            converter.str.append_view("</code>")
        }
        MdNodeKind.Text => {
            var text = node as *mut MdText
            converter.str.append_view(text.value)
        }
        MdNodeKind.Interpolation => {
            var interp = node as *mut MdInterpolation
            converter.put_chain_in()
            converter.put_wrapped_chemical_value_in(interp.value)
        }
        // TODO: implement other nodes
    }
}

func (converter : &mut MdConverter) convertMdRoot(root : *mut MdRoot) {
    converter.convertMdNode(root as *mut MdNode)
    converter.put_chain_in()
}
