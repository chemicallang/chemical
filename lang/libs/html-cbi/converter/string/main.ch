import "@std/string.ch"

func (str : &std::string) view() : std::string_view {
    return std::string_view(str.data(), str.size());
}

func make_value_chain(builder : *mut ASTBuilder, value : *mut Value, len : size_t) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const chain = builder.make_access_chain(false, location)
    var chain_values = chain.get_values()
    var base = builder.make_identifier(std::string_view("html"), false, location);
    chain_values.push(base)
    var name : std::string_view
    if(len == 0) {
        name = std::string_view("append_char_ptr")
    } else {
        name = std::string_view("append_with_len")
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

func make_chain_of(builder : *mut ASTBuilder, str : &mut std::string) : *mut AccessChain {
    const location = compiler::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(str.view()), location)
    const size = str.size()
    str.clear();
    return make_value_chain(builder, value, size);
}

func put_chain_in(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
    const chain = make_chain_of(builder, str);
    const wrapped = builder.make_value_wrapper(chain, parent)
    vec.push(wrapped);
}

func put_value_chain_in(builder : *mut ASTBuilder, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, value : *mut Value) {
    const chain = make_expr_chain_of(builder, value);
    const wrapped = builder.make_value_wrapper(chain, parent)
    vec.push(wrapped);
}

func convertHtmlAttribute(builder : *mut ASTBuilder, attr : *mut HtmlAttribute, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
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
                if(!str.empty()) {
                    put_chain_in(builder, vec, parent, str);
                }
                const value = attr.value as *mut ChemicalAttributeValue
                put_value_chain_in(builder, vec, parent, value)
            }
        }
    }
}

func convertHtmlChild(builder : *mut ASTBuilder, child : *mut HtmlChild, vec : *mut VecRef<ASTNode>, parent : *mut ASTNode, str : &mut std::string) {
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
            var a = 0;
            var attrs = element.attributes.size()
            while(a < attrs) {
                var attr = element.attributes.get(a)
                convertHtmlAttribute(builder, attr, vec, parent, str);
                a++
            }

            str.append('>')

            // doing children
            var i = 0;
            var s = element.children.size();
            while(i < s) {
                var nested_child = element.children.get(i)
                convertHtmlChild(builder, nested_child, vec, parent, str)
                i++;
            }

            str.append('<')
            str.append('/')
            str.append_with_len(element.name.data(), element.name.size())
            str.append('>')
        }
        HtmlChildKind.ChemicalValue => {
            if(!str.empty()) {
                put_chain_in(builder, vec, parent, str);
            }
            const chem_child = child as *mut HtmlChemValueChild
            put_value_chain_in(builder, vec, parent, chem_child.value)
        }
    }
}

func convertHtmlRoot(builder : *mut ASTBuilder, root : *mut HtmlRoot, vec : *mut VecRef<ASTNode>, str : &mut std::string) {
    convertHtmlChild(builder, root.element, vec, root.parent, str);
    if(!str.empty()) {
        put_chain_in(builder, vec, root.parent, str);
    }
}