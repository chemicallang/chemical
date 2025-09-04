
func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

struct ASTConverter {

    var builder : *mut ASTBuilder

    var support : *mut SymResSupport

    var vec : *mut VecRef<ASTNode>

    var parent : *mut ASTNode

    var str : std::string

}

func (converter : &mut ASTConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const support = converter.support;
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view = std::string_view("append_html_char")
    var id = builder.make_identifier(name, support.appendHtmlCharFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

func (converter : &mut ASTConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_html_char")
    var node = converter.support.appendHtmlCharFn
    var id = builder.make_identifier(name, node, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut ASTConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_html_char_ptr")
    } else {
        name = std::string_view("append_html")
    }
    var node : *mut ASTNode
    if(len == 0) {
        node = converter.support.appendHtmlCharPtrFn
    } else {
        node = converter.support.appendHtmlFn
    }
    var id = builder.make_identifier(name, node, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_ubigint_value(len, location));
    }
    return call;
}

func (converter : &mut ASTConverter) make_chain_of(str : &mut std::string) : *mut FunctionCallNode {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return converter.make_value_call(value, size);
}

func (converter : &mut ASTConverter) put_char_chain(value : char) {
    const chain = converter.make_char_chain(value);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_chain_in() {
    const chain = converter.make_chain_of(converter.str);
    converter.vec.push(chain);
}

func (converter : &mut ASTConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

var empty_string_val : *mut StringValue = null

func get_string_val(builder : *mut ASTBuilder) : *mut StringValue {
    if(empty_string_val != null) {
        return empty_string_val
    }
    const loc = intrinsics::get_raw_location();
    empty_string_val = builder.make_string_value(view(""), loc);
    return empty_string_val;
}

func (converter : &mut ASTConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    const builder = converter.builder
    var chain = converter.make_value_call(value, 0);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    const builder = converter.builder
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut ASTConverter) put_func_call_value(call : *mut FunctionCall, value : *mut Value) {
    switch(call.getType().getKind()) {
        BaseTypeKind.Void => {
            converter.put_wrapping(value);
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut ASTConverter) put_chemical_value_in(value_ptr : *mut Value) {
    var str = &converter.str
    var builder = converter.builder
    var parent = converter.parent
    var vec = converter.vec

    var value = value_ptr;

    const kind = value.getKind();
    if(kind == ValueKind.AccessChain) {
        const chain = value as *mut AccessChain
        const values = chain.get_values();
        const size = values.size();
        const last = values.get(size - 1)
        if(last.getKind() == ValueKind.FunctionCall) {
            converter.put_func_call_value(last as *mut FunctionCall, value)
        } else {
            converter.put_wrapped_chemical_value_in(value);
        }
    } else if(kind == ValueKind.FunctionCall) {
        converter.put_func_call_value(value as *mut FunctionCall, value)
    } else if(kind == ValueKind.Char) {
        converter.put_wrapped_chemical_char_value_in(value)
    } else {
        converter.put_wrapped_chemical_value_in(value);
    }
}

func (converter : &mut ASTConverter) convertHtmlAttribute(attr : *mut HtmlAttribute) {
    var str = &converter.str
    var builder = converter.builder
    var parent = converter.parent
    var vec = converter.vec

    str.append(' ')
    str.append_with_len(attr.name.data(), attr.name.size())
    if(attr.value != null) {
        str.append('=')
        switch(attr.value.kind) {
            AttributeValueKind.Text, AttributeValueKind.Number => {
                const value = attr.value as *mut TextAttributeValue
                str.append_with_len(value.text.data(), value.text.size())
            }
            AttributeValueKind.Chemical => {
                str.append('"');
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                const value = attr.value as *mut ChemicalAttributeValue
                converter.put_chemical_value_in(value.value)
                str.append('"');
            }
            AttributeValueKind.ChemicalValues => {
                if(!str.empty()) {
                    converter.put_chain_in();
                }
                converter.put_char_chain('"');
                const value = attr.value as *mut ChemicalAttributeValues
                const size = value.values.size();
                const last = size - 1;
                var i : uint = 0;
                while(i < size) {
                    converter.put_chemical_value_in(value.values.get(i))
                    if(i != last) {
                        converter.put_char_chain(' ');
                    }
                    i++;
                }
                converter.put_char_chain('"');
            }
        }
    }
}

func (converter : &mut ASTConverter) convertHtmlChild(child : *mut HtmlChild) {

    var str = &converter.str
    var builder = converter.builder
    var parent = converter.parent
    var vec = converter.vec

    switch(child.kind) {
        HtmlChildKind.Text => {
            var text = child as *mut HtmlText
            str.append_with_len(text.value.data(), text.value.size());
        }
        HtmlChildKind.Element => {
            var element = child as *mut HtmlElement
            str.append('<')
            str.append_with_len(element.name.data(), element.name.size())

            // putting attributes
            var a : uint = 0;
            var attrs = element.attributes.size()
            while(a < attrs) {
                var attr = element.attributes.get(a)
                converter.convertHtmlAttribute(attr);
                a++
            }

            if(element.isSelfClosing) {
                str.append('/');
            }

            str.append('>')

            // doing children
            var i : uint = 0;
            var s = element.children.size();
            while(i < s) {
                var nested_child = element.children.get(i)
                converter.convertHtmlChild(nested_child)
                i++;
            }

            if(!element.isSelfClosing) {
                str.append('<')
                str.append('/')
                str.append_with_len(element.name.data(), element.name.size())
                str.append('>')
            }

        }
        HtmlChildKind.Comment => {
            // we can just skip comments
            // TODO provide an option to write out comments
        }
        HtmlChildKind.ChemicalValue => {
            if(!str.empty()) {
                converter.put_chain_in();
            }
            const chem_child = child as *mut HtmlChemValueChild
            converter.put_chemical_value_in(chem_child.value)
        }
    }
}

func (converter : &mut ASTConverter) convertHtmlRoot(root : *mut HtmlRoot) {
    if(!root.children.empty()) {
        var i = 0;
        const total = root.children.size();
        while(i < total) {
            const child = root.children.get(i as size_t)
            converter.convertHtmlChild(child);
            i++;
        }
        if(!converter.str.empty()) {
            converter.put_chain_in();
        }
    }
}