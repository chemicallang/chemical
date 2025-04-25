
func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

func make_char_chain(builder : *mut ASTBuilder, value : char) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("page"), false, location);
    chain_values.push(base)
    var name : std::string_view = std::string_view("append_html_char")
    var id = builder.make_identifier(name, false, location);
    chain_values.push(id)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    const new_chain = builder.make_access_chain(true, location)
    var new_chain_values = new_chain.get_values();
    new_chain_values.push(call);
    return new_chain;
}

func make_value_chain(builder : *mut ASTBuilder, value : *mut Value, len : size_t) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("page"), false, location);
    chain_values.push(base)
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_html_char_ptr")
    } else {
        name = std::string_view("append_html")
    }
    var id = builder.make_identifier(name, false, location);
    chain_values.push(id)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    if(len != 0) {
        args.push(builder.make_number_value(len, location));
    }
    const new_chain = builder.make_access_chain(true, location)
    var new_chain_values = new_chain.get_values();
    new_chain_values.push(call);
    return new_chain;
}

func make_expr_chain_of(builder : *mut ASTBuilder, value : *mut Value) : *mut AccessChain {
    return make_value_chain(builder, value, 0);
}

func replace_value_in_expr_chain(builder : *mut ASTBuilder, chain : *mut AccessChain, new_value : *mut Value) {
    const values = chain.get_values();
    const last = values.get(values.size() - 1)
    const call = last as *mut FunctionCall
    const args = call.get_args();
    args.set(args.size() - 1, new_value)
}

func make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return make_value_chain(builder, value, size);
}

func put_link_wrap_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, chain : *mut AccessChain) {
    var wrapped = builder.make_value_wrapper(chain, parent)
    wrapped.declare_and_link(&wrapped, resolver);
    vec.push(wrapped);
}

func put_char_chain(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : char) {
    const chain = make_char_chain(builder, value);
    put_link_wrap_chain(resolver, builder, vec, parent, chain);
}

func put_chain_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    const chain = make_chain_of(builder, str);
    var wrapped = builder.make_value_wrapper(chain, parent)
    wrapped.declare_and_link(&wrapped, resolver);
    vec.push(wrapped);
}

func put_wrapping(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    const wrapped = builder.make_value_wrapper(value, parent)
    vec.push(wrapped);
}

var empty_string_val : *mut StringValue = null

func get_string_val(builder : *mut ASTBuilder) : *mut StringValue {
    if(empty_string_val != null) {
        return empty_string_val
    }
    const loc = compiler::get_raw_location();
    empty_string_val = builder.make_string_value(view(""), loc);
    return empty_string_val;
}

// we link the expression chain with a dummy string value (to already linked value to be linked twice)
// then we replace the value in expression chain
func put_wrapped_chemical_value_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    // first we link the expression chain with dummy empty string value
    var chain = make_expr_chain_of(builder, get_string_val(builder));
    // link the chain
    chain.link(&chain, null, resolver)
    // replace the value
    replace_value_in_expr_chain(builder, chain, value)
    // then we replace the dummy string value with actual linked value
    put_wrapping(builder, vec, parent, chain)
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

func put_chemical_value_in(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value_ptr : *mut Value) {
    var value = value_ptr;
    if(!value.link(&value, null, resolver)) {
        put_wrapping(builder, vec, parent, value);
        return;
    }
    const kind = value.getKind();
    if(kind == ValueKind.AccessChain) {
        const chain = value as *mut AccessChain
        const values = chain.get_values();
        const size = values.size();
        printf("received size of access chain %d\n", size)
        fflush(null)
        const last = values.get(size - 1)
        if(last.getKind() == ValueKind.FunctionCall) {
            if(is_func_call_ret_void(builder, last as *mut FunctionCall)) {
                put_wrapping(builder, vec, parent, value);
            } else {
                put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
            }
        } else {
            put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
        }
    } else if(kind == ValueKind.FunctionCall) {
        if(is_func_call_ret_void(builder, value as *mut FunctionCall)) {
            put_wrapping(builder, vec, parent, value);
        } else {
            put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
        }
    } else {
        put_wrapped_chemical_value_in(resolver, builder, vec, parent, value);
    }
}

func convertHtmlAttribute(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, attr : *mut HtmlAttribute, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
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
                    put_chain_in(resolver, builder, vec, parent, str);
                }
                const value = attr.value as *mut ChemicalAttributeValue
                put_chemical_value_in(resolver, builder, vec, parent, value.value)
                str.append('"');
            }
            AttributeValueKind.ChemicalValues => {
                if(!str.empty()) {
                    put_chain_in(resolver, builder, vec, parent, str);
                }
                put_char_chain(resolver, builder, vec, parent, '"');
                const value = attr.value as *mut ChemicalAttributeValues
                const size = value.values.size();
                const last = size - 1;
                var i : uint = 0;
                while(i < size) {
                    put_chemical_value_in(resolver, builder, vec, parent, value.values.get(i))
                    if(i != last) {
                        put_char_chain(resolver, builder, vec, parent, ' ');
                    }
                    i++;
                }
                put_char_chain(resolver, builder, vec, parent, '"');
            }
        }
    }
}

func convertHtmlChild(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, child : *mut HtmlChild, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
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
                convertHtmlAttribute(resolver, builder, attr, vec, parent, str);
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
                convertHtmlChild(resolver, builder, nested_child, vec, parent, str)
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
                put_chain_in(resolver, builder, vec, parent, str);
            }
            const chem_child = child as *mut HtmlChemValueChild
            put_chemical_value_in(resolver, builder, vec, parent, chem_child.value)
        }
    }
}

func convertHtmlRoot(resolver : *mut SymbolResolver, builder : *mut ASTBuilder, root : *mut HtmlRoot, vec : *mut VecRef<ASTNode>, str : &mut std::string) {
    convertHtmlChild(resolver, builder, root.element, vec, root.parent, str);
    if(!str.empty()) {
        put_chain_in(resolver, builder, vec, root.parent, str);
    }
}