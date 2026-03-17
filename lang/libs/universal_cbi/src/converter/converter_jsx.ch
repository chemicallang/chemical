func (converter : &mut JsConverter) convertAttributeValue(attr : *mut JsJSXAttribute) {
    if(attr.value != null) {
         if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
             const container = attr.value as *mut JsJSXExpressionContainer
             if(container.expression != null) {
                 if(container.expression.kind == JsNodeKind.ChemicalValue) {
                     const cv = container.expression as *mut JsChemicalValue;
                     const cvType = cv.value.getType();
                     const isStr = cvType != null && (cvType.getKind() == BaseTypeKind.String ||
                         (cvType.getKind() == BaseTypeKind.Pointer) ||
                         (cvType.getKind() == BaseTypeKind.ExpressiveString));
                     if(isStr) {
                         converter.str.append('"');
                         converter.put_chain_in();
                         converter.put_chemical_value_in(cv.value);
                         converter.str.append('"');
                         return;
                     }
                 }
                 if(container.expression.kind == JsNodeKind.Identifier) {
                     const id = container.expression as *mut JsIdentifier
                     if(converter.is_state_var(id.value)) {
                         converter.str.append_view(id.value);
                         return;
                     }
                 } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                     const mem = container.expression as *mut JsMemberAccess
                     if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                         const id = mem.object as *mut JsIdentifier
                         if(converter.is_state_var(id.value)) {
                             converter.str.append_view(id.value);
                             return;
                         }
                     }
                 }
             }
         }
         converter.convertJsNode(attr.value);
    } else {
         converter.str.append_view("true");
    }
}

func (converter : &mut JsConverter) convertJSXComponent(element : *mut JsJSXElement, tagName : std::string_view, tagNameNode : *mut JsNode) {
    if(converter.target == BufferType.HTML) {
        if(element.componentSignature == null) {
             return;
        }

        const signature = element.componentSignature;
        const hash = signature.functionNode.getEncodedLocation();

        var body = converter.vec

        var base = converter.builder.make_identifier(signature.name, signature.functionNode, false, intrinsics::get_raw_location())
        var pageId = converter.builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, intrinsics::get_raw_location())
        var call = converter.builder.make_function_call_node(base, converter.parent, intrinsics::get_raw_location())
        call.get_args().push(pageId)
        
        const attrs = converter.build_ssr_attributes(element);
        call.get_args().push(converter.builder.make_addr_of_value(attrs, intrinsics::get_raw_location()));

        const location = intrinsics::get_raw_location();
        const builder = converter.builder;

        // if the children is empty, we pass empty ssr text
        if(element.children.empty()) {
            // 4. Construct SsrText and pass as 3rd arg
            const ssrTextStructVal = converter.builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
            ssrTextStructVal.add_value("data", builder.make_null_value(location));
            ssrTextStructVal.add_value("size", builder.make_ubigint_value(0, location));
            call.get_args().push(ssrTextStructVal);
            body.push(call)
            return;
        }

        // and now the children argument
        // we render the children into page
        // then take the page's html string and pass it to child component

        var getSizeCall = builder.make_function_call_value(
            builder.make_access_chain(std::span<*mut Value>([ pageId, builder.make_identifier(std::string_view("get_html_size"), converter.support.getHtmlSizeFn, false, location) ]), location),
            location
        );
        var startIdxNameStr = std::string();
        startIdxNameStr.append_view("startIdx_");
        startIdxNameStr.append_integer(location as bigint);
        var startIdxName = builder.allocate_view(startIdxNameStr.to_view());

        var startIdxVar = builder.make_varinit_stmt(false, false, startIdxName, builder.get_u64_type(), getSizeCall, AccessSpecifier.Internal, converter.parent, location);
        body.push(startIdxVar);

        // 3. Extract range and truncate
        var pageId2 = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var pageHtmlAccess = builder.make_access_chain(std::span<*mut Value>([ pageId2, builder.make_identifier(std::string_view("pageHtml"), converter.support.pageHtmlNode, false, location) ]), location);

        var childrenHtmlNameStr = std::string();
        childrenHtmlNameStr.append_view("childrenHtml_");
        childrenHtmlNameStr.append_integer(location as bigint);
        var childrenHtmlName = builder.allocate_view(childrenHtmlNameStr.to_view());

        var childrenHtmlVar = builder.make_varinit_stmt(false, false, childrenHtmlName, null, 
            builder.make_function_call_value(builder.make_identifier(view("std::string"), converter.support.stringNodeMake, false, location), location),
            AccessSpecifier.Internal, converter.parent, location);
        body.push(childrenHtmlVar);

        // 2. Render children
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.convertJsNode(element.children.get(i));
        }

        var childrenHtmlId = builder.make_identifier(childrenHtmlName, childrenHtmlVar , false, location);
        var appendCall = builder.make_function_call_node(
            builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("append_with_len"), converter.support.appendWithLenFn, false, location) ]), location),
            converter.parent,
            location
        );
        var startIdxId = builder.make_identifier(startIdxName, startIdxVar , false, location);
        var dataCall = builder.make_function_call_value(
            builder.make_access_chain(std::span<*mut Value>([ pageHtmlAccess as *mut Value, builder.make_identifier(view("data"), converter.support.dataFn, false, location) ]), location),
            location
        );
        var sizeCall = builder.make_function_call_value(
            builder.make_access_chain(std::span<*mut Value>([ pageHtmlAccess as *mut Value, builder.make_identifier(view("size"), converter.support.sizeFn, false, location) ]), location),
            location
        );
        
        appendCall.get_args().push(builder.make_expression_value(dataCall, startIdxId, Operation.Addition, dataCall.getType(), location));
        appendCall.get_args().push(builder.make_expression_value(sizeCall, startIdxId, Operation.Subtraction, sizeCall.getType(), location));
        body.push(appendCall);

        var pageId3 = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
        var truncateCall = builder.make_function_call_node(
            builder.make_access_chain(std::span<*mut Value>([ pageId3, builder.make_identifier(view("truncate_html"), converter.support.truncateHtmlFn, false, location) ]), location),
            converter.parent,
            location
        );
        truncateCall.get_args().push(builder.make_identifier(startIdxName, startIdxVar , false, location));
        body.push(truncateCall );

        // 4. Construct SsrText and pass as 3rd arg
        const ssrTextStructVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
        
        var dataCall2 = builder.make_function_call_value(
            builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("data"), converter.support.dataFn, false, location) ]), location),
            location
        );
        var sizeCall2 = builder.make_function_call_value(
            builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("size"), converter.support.sizeFn, false, location) ]), location),
            location
        );
        
        ssrTextStructVal.add_value("data", dataCall2);
        ssrTextStructVal.add_value("size", sizeCall2);
        
        call.get_args().push(ssrTextStructVal);
        
        body.push(call )

        return;
    }

    converter.str.append_view("$_ur.createElement(");

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        if(element.componentSignature != null) {
            get_module_scoped_name(element.componentSignature.functionNode , tagName, converter.str);
        } else {
            converter.str.append_view(tagName);
        }
    } else {
        converter.convertJsNode(tagNameNode);
    }

    converter.str.append_view(", {");

    var attrCount = 0u;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i)
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const attr = attrNode as *mut JsJSXAttribute
            var name = attr.name;
            if(name.equals("class")) name = std::string_view("className");
            else if(name.equals("for")) name = std::string_view("htmlFor");

            converter.str.append_view("\"");
            converter.str.append_view(name);
            converter.str.append_view("\": ");
            converter.convertAttributeValue(attr);
            attrCount++;
        } else if (attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
            if(attrCount > 0) converter.str.append_view(", ");
            const spread = attrNode as *mut JsJSXSpreadAttribute
            converter.str.append_view("...");
            converter.convertJsNode(spread.argument);
            attrCount++;
        }
    }

    converter.str.append_view("}");

    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }

    converter.str.append_view(")");
}

func (converter : &mut JsConverter) convertJSXNativeElement(element : *mut JsJSXElement, tagName : std::string_view) {
    if(converter.target == BufferType.HTML) {

        converter.str.append('<');
        converter.str.append_view(tagName);
        converter.put_chain_in();

        const builder = converter.builder;
        const location = intrinsics::get_raw_location();
        const support = converter.support;

        if(!element.opening.attributes.empty()) {
            // Use render.ch's build_ssr_attributes logic but adapted for this context
            // Actually, build_ssr_attributes should be a method on JsConverter
            const attrs = converter.build_ssr_attributes(element);
            var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
            var call = builder.make_function_call_node(builder.make_identifier(std::string_view("renderHtmlAttrs"), support.renderHtmlAttrs, false, location), converter.parent, location);
            call.get_args().push(pageId);
            call.get_args().push(attrs);
            converter.vec.push(call );
        }

        converter.str.append('>');
        converter.put_chain_in();

        for(var i : uint = 0; i < element.children.size(); i++) {
            converter.convertJsNode(element.children.get(i));
        }

        converter.str.append_view("</");
        converter.str.append_view(tagName);
        converter.str.append('>');
        converter.put_chain_in();
        return;
    }

    converter.str.append_view("$_ur.createElement(\"");
    converter.str.append_view(tagName);
    converter.str.append_view("\", ");

     var hasSpread = false;
     var attrMap = std::vector<*mut JsJSXAttribute>();
     for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
         const attrNode = element.opening.attributes.get(i);
         if(attrNode.kind == JsNodeKind.JSXAttribute) {
             attrMap.push(attrNode as *mut JsJSXAttribute);
         } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
             hasSpread = true;
         }
     }

     if(hasSpread) {
         // Use merge helper $_um
         converter.str.append_view("$_um(");
         var first = true;
         for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
             const attrNode = element.opening.attributes.get(i);
             if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                 if(!first) converter.str.append_view(", ");
                 const spread = attrNode as *mut JsJSXSpreadAttribute;
                 converter.convertJsNode(spread.argument);
                 first = false;
             }
         }
         // Group static/named attributes into an object for merging
         if(!attrMap.empty()) {
             if(!first) converter.str.append_view(", ");
             converter.str.append_view("{");
             for(var i : uint = 0; i < attrMap.size(); i++) {
                 if(i > 0) converter.str.append_view(", ");
                 const attr = attrMap.get(i);
                 converter.str.append_view("\"");
                 converter.str.append_view(attr.name);
                 converter.str.append_view("\": ");
                 converter.convertAttributeValue(attr);
             }
             converter.str.append_view("}");
         }
         converter.str.append_view(")");
     } else {
         converter.str.append_view("{");
         var first = true;
         for(var i : uint = 0; i < attrMap.size(); i++) {
             const attr = attrMap.get(i);
             if(!first) converter.str.append_view(", ");
             converter.str.append_view("\"");
             converter.str.append_view(attr.name);
             converter.str.append_view("\": ");
             converter.convertAttributeValue(attr);
             first = false;
         }
         converter.str.append_view("}");
     }

    if(!element.children.empty()) {
        for(var i : uint = 0; i < element.children.size(); i++) {
             converter.str.append_view(", ");
             converter.convertJsNode(element.children.get(i));
        }
    }

    converter.str.append_view(")");
}

func (converter : &mut JsConverter) convertJSXElement(element : *mut JsJSXElement) {
    const tagNameNode = element.opening.tagName as *mut JsNode
    var isComponent = false
    var tagName = std::string_view()

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        const jsId = tagNameNode as *mut JsIdentifier
        tagName = jsId.value
        if(element.componentSignature != null || (tagName.size() > 0 && tagName.get(0) >= 'A' && tagName.get(0) <= 'Z')) {
            isComponent = true
        }
    } else if(tagNameNode.kind == JsNodeKind.JSXExpressionContainer) {
        isComponent = true
    }

    if(isComponent) {
        converter.convertJSXComponent(element, tagName, tagNameNode)
    } else {
        converter.convertJSXNativeElement(element, tagName)
    }
}

func (converter : &mut JsConverter) convertJSXFragment(fragment : *mut JsJSXFragment) {
    if(converter.target == BufferType.HTML) {
        for(var i : uint = 0; i < fragment.children.size(); i++) {
             converter.convertJsNode(fragment.children.get(i));
        }
        return;
    }

    converter.str.append_view("$_ur.createElement($_ur.Fragment, null");

    for(var i : uint = 0; i < fragment.children.size(); i++) {
          converter.str.append_view(", ");
          converter.convertJsNode(fragment.children.get(i));
    }

    converter.str.append_view(")");
}
