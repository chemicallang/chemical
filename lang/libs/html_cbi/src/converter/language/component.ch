func (converter : &mut ASTConverter) convertHtmlComponent(element : *mut HtmlElement) {
    // 0. Flush any pending HTML
    converter.put_chain_in()

    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const signature = element.componentSignature

    // the output string
    var s = &mut converter.str

    // 1. Generate the hash based on component name
    const hash = signature.functionNode.getEncodedLocation()

    // 2. Generate the if(page.require_component(hash)) block to emit component JS.
    if(signature.mountStrategy != MountStrategy.Universal) {
        var requireCall = converter.make_require_component_call(hash as size_t)
        var ifStmt = builder.make_if_stmt(requireCall as *mut Value, converter.parent, location)
        var body = ifStmt.get_body()
        body.push(converter.make_set_component_hash_call(hash as size_t))

        var base = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var call = builder.make_function_call_node(base as *mut Value, converter.parent, location)
        call.get_args().push(pageId as *mut Value)
        body.push(call as *mut ASTNode)

        converter.vec.push(ifStmt as *mut ASTNode)
    }

    // special case for universal component rendering
    // it just requires a single function call with the props
    if(signature.mountStrategy == MountStrategy.Universal) {

        // lets construct a var decl array of attributes
        // var attrs = [ SsrAttribute { name : SsrText { data : "", size : 0 }, value : SsrAttributeValue.Text(SsrText { data : "", size : 0 }) } ]

        // the ssr attribute linked type
        const ssrAttrLinkedNode = converter.support.ssrAttrLinkedNode
        const ssrAttrLinkedType = builder.make_linked_type(std::string_view("SsrAttribute"), ssrAttrLinkedNode, location)

        // the ssr text linked type
        const ssrTextLinkedNode = converter.support.ssrTextLinkedNode
        const ssrTextLinkedType = builder.make_linked_type(std::string_view("SsrText"), ssrTextLinkedNode, location)

        // creating an array value for the attributes
        const arrayValue = builder.make_array_value(ssrAttrLinkedType, location)
        const attrValues = arrayValue.get_values()

        var attrValConv = AttrValueConverter {
            pageNode : converter.support.pageNode,
            ssrAttributeValueNode : converter.support.ssrAttributeValueNode,
            multipleAttributeValueNode : converter.support.multipleAttributeValueNode,
            parent : converter.parent
        }

        // constructing ssr attributes
        for(var i = 0u; i < element.attributes.size(); i++) {
            const attr = element.attributes.get(i)

            // constructing a ssr text for the attribute name
            const attrStructVal = builder.make_struct_value(ssrAttrLinkedType, converter.parent, location)

            // constructing a ssr text val for the name value
            const nameStructVal = builder.make_struct_value(ssrTextLinkedType, converter.parent, location)
            nameStructVal.add_value(std::string_view("data"), builder.make_string_value(attr.name, location))
            nameStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(attr.name.size(), location))
            attrStructVal.add_value(std::string_view("name"), nameStructVal)

            // constructing a ssr value
            const attrValue = attr.value;
            if(attrValue.kind == AttributeValueKind.Chemical) {
                var chemAttrValue = attrValue as *mut ChemicalAttributeValue
                var attrValueVal = attrValConv.convert_to_attr_value(builder, chemAttrValue.value.getType(), chemAttrValue.value)

                attrStructVal.add_value(std::string_view("value"), attrValueVal);
            } else if(attrValue.kind == AttributeValueKind.ChemicalValues) {
                var chemAttrValue = attrValue as *mut ChemicalAttributeValues
                const multiVal = attrValConv.convert_multiple_attr_values(builder, chemAttrValue.values.data(), chemAttrValue.values.size())
                attrStructVal.add_value(std::string_view("value"), multiVal);

            } else {
                // constructing a ssr text val for the name value
                var chemAttrValue = attrValue as *mut TextAttributeValue
                const textStructVal = builder.make_struct_value(ssrTextLinkedType, converter.parent, location)
                textStructVal.add_value(std::string_view("data"), builder.make_string_value(chemAttrValue.text, location))
                textStructVal.add_value(std::string_view("size"), builder.make_ubigint_value(chemAttrValue.text.size(), location))

                attrStructVal.add_value(std::string_view("value"), textStructVal);
            }

            // putting the attribute struct val into the array
            attrValues.push(attrStructVal)

        }

        // Call ComponentFunction(page) to write the component's HTML
        var compBase = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
        var compPageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var compCall = builder.make_function_call_node(compBase as *mut Value, converter.parent, location)
        const args = compCall.get_args();
        args.push(compPageId as *mut Value)

        // creating a struct value for ssr attribute list
        const ssrAttributeListNode = converter.support.ssrAttributeListNode
        const ssrAttributeListNodeType = builder.make_linked_type(std::string_view("SsrAttributeList"), ssrAttributeListNode, location)
        const structValue = builder.make_struct_value(ssrAttributeListNodeType, converter.parent, location)

        // now lets add value for the data (going to be the array), and size
        structValue.add_value(std::string_view("data"), arrayValue)
        structValue.add_value(std::string_view("size"), builder.make_ubigint_value(element.attributes.size(), location))

        // make the struct value the second argument
        args.push(structValue)

        // put the call in the vec
        converter.vec.push(compCall as *mut ASTNode)
        return;
    }

    s.append_view("<script>")

    if(signature.mountStrategy == MountStrategy.Preact) {
        // Preact Mount Strategy
        s.append_view("$_pm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.React) {
        // React Mount Strategy
        s.append_view("$_rm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.Solid) {
        // Solid Mount Strategy
        s.append_view("$_sm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else if(signature.mountStrategy == MountStrategy.Universal) {
        // Universal Mount Strategy (HTML-first, no framework dependency)
        s.append_view("$_um(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    } else {
        // Default Mount Strategy
        s.append_view("$_dm(document.currentScript, ")
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, *s)
        s.append_view(", {")
    }

    const attrs = element.attributes.size()
    for (var i : uint = 0; i < attrs; i++) {
        if (i > 0) s.append_view(", ")
        const attr = element.attributes.get(i)
        s.append_view(attr.name)
        s.append_view(": ")

        if (attr.value != null) {
            switch(attr.value.kind) {
                AttributeValueKind.Text, AttributeValueKind.Number => {
                    const val = attr.value as *mut TextAttributeValue
                    s.append_view(val.text)
                }
                AttributeValueKind.Chemical => {
                    const val = attr.value as *mut ChemicalAttributeValue
                    const type = val.value.getType()
                    const is_str = converter.is_string_type(type)

                    if(is_str) {
                         s.append('"')
                    }

                    converter.emit_append_html_from_str(*s)

                    converter.put_chemical_value_in(val.value)

                    if(is_str) {
                        converter.put_char_chain('"')
                    }
                }
                AttributeValueKind.ChemicalValues => {
                    converter.emit_append_html_from_str(*s)
                    const valuesNode = attr.value as *mut ChemicalAttributeValues
                    converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                    for (var j : uint = 0; j < valuesNode.values.size(); j++) {
                        if (j > 0) converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view(" ")), location), 1)
                        converter.put_chemical_value_in(valuesNode.values.get(j))
                    }
                    converter.emit_append_html_call(builder.make_string_value(builder.allocate_view(std::string_view("\"")), location), 1)
                }
            }
        } else {
            s.append_view("true")
        }
    }

    s.append_view("});")

    s.append_view("</script>")
    converter.emit_append_html_from_str(*s)
}