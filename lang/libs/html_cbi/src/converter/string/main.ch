
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

func (converter : &mut ASTConverter) make_char_chain(value : char) : *mut AccessChain {
    const builder = converter.builder
    const support = converter.support;
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var name : std::string_view = std::string_view("append_html_char")
    var id = builder.make_identifier(name, support.appendHtmlCharFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    const new_chain = builder.make_access_chain(std::span<*mut ChainValue>([ call ]), location)
    return new_chain;
}

func (converter : &mut ASTConverter) make_value_chain(value : *mut Value, len : size_t) : *mut AccessChain {
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
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    const new_chain = builder.make_access_chain(std::span<*mut ChainValue>([ call ]), location)
    return new_chain;
}

func (converter : &mut ASTConverter) make_expr_chain_of(value : *mut Value) : *mut AccessChain {
    return converter.make_value_chain(value, 0);
}

func (converter : &mut ASTConverter) make_chain_of(str : &mut std::string) : *mut AccessChain {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return converter.make_value_chain(value, size);
}

func (converter : &mut ASTConverter) put_link_wrap_chain(chain : *mut AccessChain) {
    var wrapped = converter.builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut ASTConverter) put_char_chain(value : char) {
    const chain = converter.make_char_chain(value);
    converter.put_link_wrap_chain(chain);
}

func (converter : &mut ASTConverter) put_chain_in() {
    const chain = converter.make_chain_of(converter.str);
    var wrapped = converter.builder.make_value_wrapper(chain, converter.parent)
    converter.vec.push(wrapped);
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
    var chain = converter.make_expr_chain_of(value);
    converter.put_wrapping(chain)
}

func is_func_call_ret_void(builder : *mut ASTBuilder, call : *mut FunctionCall) : bool {
    const type = builder.createType(call)
    if(type) {
        const kind = type.getKind();
        return kind == BaseTypeKind.Void;
    } else {
        return false;
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
            if(is_func_call_ret_void(builder, last as *mut FunctionCall)) {
                converter.put_wrapping(value);
            } else {
                converter.put_wrapped_chemical_value_in(value);
            }
        } else {
            converter.put_wrapped_chemical_value_in(value);
        }
    } else if(kind == ValueKind.FunctionCall) {
        if(is_func_call_ret_void(builder, value as *mut FunctionCall)) {
            converter.put_wrapping(value);
        } else {
            converter.put_wrapped_chemical_value_in(value);
        }
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
    if(root.element != null) {
        converter.convertHtmlChild(root.element);
        if(!converter.str.empty()) {
            converter.put_chain_in();
        }
    }
}