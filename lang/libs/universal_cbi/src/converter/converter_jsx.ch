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
             // Fallback to JS if no signature (maybe partial match or dynamic?)
             return;
        }

        const signature = element.componentSignature;
        const hash = signature.functionNode.getEncodedLocation();

        // Marker for hydration
        var idStr = std::string();
        idStr.append_view("u");
        idStr.append_uinteger(element.loc);

        converter.str.append_view("<div id=\"");
        converter.str.append_view(idStr.to_view());
        converter.str.append_view("\" data-u-comp=\"");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view("\">");
        converter.put_chain_in();

        // 1. Emit the require/if check and C++ call for SSR
        // This makes it work like html_cbi's components.
        var requireCall = converter.make_require_component_call(hash as size_t)
        var ifStmt = converter.builder.make_if_stmt(requireCall as *mut Value, converter.parent, intrinsics::get_raw_location())
        var body = ifStmt.get_body()

        body.push(converter.make_set_component_hash_call(hash as size_t))

        var base = converter.builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, intrinsics::get_raw_location())
        var pageId = converter.builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, intrinsics::get_raw_location())
        var call = converter.builder.make_function_call_node(base, converter.parent, intrinsics::get_raw_location())
        call.get_args().push(pageId as *mut Value)

        // Push props (this is tricky, we need to convert JSX attributes to C++ struct initializer)
        // For now, only passing page.

        body.push(call as *mut ASTNode)
        converter.vec.push(ifStmt as *mut ASTNode)

        // 2. Emit hydration entry for JS
        // This should go to pageJs.
        const oldTarget = converter.target;
        converter.target = BufferType.JavaScript;
        converter.str.append_view("window.$_uq.push(['");
        converter.str.append_view(idStr.to_view());
        converter.str.append_view("','");
        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
        converter.str.append_view("',{");
        var first = true;
        for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
            const attrNode = element.opening.attributes.get(i);
            if(attrNode.kind == JsNodeKind.JSXAttribute) {
                if(!first) converter.str.append(',');
                const attr = attrNode as *mut JsJSXAttribute;
                converter.str.append_view("\"");
                converter.str.append_view(attr.name);
                converter.str.append_view("\":");
                // Emit attribute value — quote ChemicalValue/ExpressiveString if it's a string type
                if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const cont = attr.value as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.ChemicalValue) {
                        const cv = cont.expression as *mut JsChemicalValue;
                        const cvType = cv.value.getType();
                        const isStr = cvType != null && (cvType.getKind() == BaseTypeKind.String ||
                            (cvType.getKind() == BaseTypeKind.Pointer) ||
                            (cvType.getKind() == BaseTypeKind.ExpressiveString));
                        if(isStr) {
                            // Emit as: "+chemValue+"
                            converter.str.append_view("\"");
                            converter.put_chain_in();
                            converter.put_chemical_value_in(cv.value);
                            converter.str.append_view("\"");
                        } else {
                            converter.convertAttributeValue(attr);
                        }
                    } else {
                        converter.convertAttributeValue(attr);
                    }
                } else {
                    converter.convertAttributeValue(attr);
                }
                first = false;
            }
        }
        converter.str.append_view("}]);");
        converter.put_chain_in();
        converter.target = oldTarget;

        converter.str.append_view("</div>");
        converter.put_chain_in();
        return;
    }

    converter.str.append_view("$_ur.createElement(");

    if(tagNameNode.kind == JsNodeKind.Identifier) {
        if(element.componentSignature != null) {
            get_module_scoped_name(element.componentSignature.functionNode as *mut ASTNode, tagName, converter.str);
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
        converter.str.append(' ');
        converter.put_chain_in();

        const builder = converter.builder;
        const location = intrinsics::get_raw_location();
        const support = converter.support;

        // Use render.ch's build_ssr_attributes logic but adapted for this context
        // Actually, build_ssr_attributes should be a method on JsConverter
        const attrs = converter.build_ssr_attributes(element);
        var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
        var call = builder.make_function_call_node(builder.make_identifier(std::string_view("renderHtmlAttrs"), support.renderHtmlAttrs, false, location), converter.parent, location);
        call.get_args().push(pageId as *mut Value);
        call.get_args().push(attrs);
        converter.vec.push(call as *mut ASTNode);

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
