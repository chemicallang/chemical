@no_mangle
public func universal_symResSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
    // no type or value is included in this node which requires resolution at link signature
    // other than HtmlPage type, how are we are comfortable resolving that after
}

@no_mangle
public func universal_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsComponentDecl;

    root.htmlPageNode = resolver.find(std::string_view("HtmlPage"));
    if(root.htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    const builder = resolver.getJobBuilder()

    const voidType = builder.make_void_type(loc);
    const funcDecl = builder.make_function(root.signature.name, voidType as *mut BaseType, false, true, node.getParent(), loc);

    // func name(page : &mut HtmlPage) : void
    const linked = builder.make_linked_type(std::string_view("HtmlPage"), root.htmlPageNode, loc);
    const ref = builder.make_reference_type(linked as *mut BaseType, true, loc);
    const param = builder.make_function_param(std::string_view("page"), ref as *mut BaseType, 0, null, false, funcDecl, loc);

    funcDecl.get_params().push(param);
    funcDecl.add_body();

    // start a scope to store symbols
    resolver.scope_start();

    // declare the page param
    resolver.declare_or_shadow(std::string_view("page"), param)

    // visit the body
    visitor.visitEmbeddedNode(node)

    // resolve components
    sym_res_components(root.components, resolver)

    // end the scope
    resolver.scope_end();

    // fixing support
    root.support.pageNode = param as *mut ASTNode
    fix_support_page_node(root.support, root.support.pageNode, loc)

    root.signature.functionNode = funcDecl;

}

@no_mangle
public func universal_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}

struct UniversalStateDecl {
    var name : std::string_view
    var initExpr : std::string_view
    var initText : std::string_view
}

struct UniversalTextBinding {
    var stateName : std::string_view
    var path : std::string_view
}

struct UniversalEventBinding {
    var eventName : std::string_view
    var path : std::string_view
    var handlerExpr : std::string_view
}

func is_event_attr_name(name : std::string_view) : bool {
    return name.size() > 2 && name.get(0) == 'o' && name.get(1) == 'n';
}

func event_attr_to_dom_event(builder : *mut ASTBuilder, name : std::string_view) : std::string_view {
    if(name.size() <= 2) return name;
    var eventName = std::string();
    for(var i : uint = 2; i < name.size(); i++) {
        var c = name.get(i);
        if(c >= 'A' && c <= 'Z') {
            c = c + 32;
        }
        eventName.append(c);
    }
    return builder.allocate_view(eventName.to_view());
}

func is_native_tag(tag : std::string_view) : bool {
    return !tag.empty() && tag.get(0) >= 'a' && tag.get(0) <= 'z';
}

func escape_html_append(out : &mut std::string, text : std::string_view) {
    for(var i : uint = 0; i < text.size(); i++) {
        const c = text.get(i);
        switch(c) {
            '&' => out.append_view("&amp;")
            '<' => out.append_view("&lt;")
            '>' => out.append_view("&gt;")
            '"' => out.append_view("&quot;")
            '\'' => out.append_view("&#39;")
            default => out.append(c)
        }
    }
}

func strip_js_string_quotes(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.get(0);
        const last = value.get(value.size() - 1);
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            return value.subview(1, value.size() - 1);
        }
    }
    return value;
}

func find_state_init_text(states : &std::vector<UniversalStateDecl>, name : std::string_view) : std::string_view {
    for(var i : uint = 0; i < states.size(); i++) {
        const st = states.get(i);
        if(st.name.equals(name)) {
            return st.initText;
        }
    }
    return view("");
}

func has_state(states : &std::vector<UniversalStateDecl>, name : std::string_view) : bool {
    for(var i : uint = 0; i < states.size(); i++) {
        if(states.get(i).name.equals(name)) return true;
    }
    return false;
}

func js_node_to_source(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    states : &std::vector<UniversalStateDecl>
) : std::string_view {
    var tmpSupport = SymResSupport {}
    var tmpConv = JsConverter {
        builder : builder,
        support : &mut tmpSupport,
        vec : null,
        parent : null,
        str : std::string(),
        jsx_parent : view(""),
        t_counter : 0,
        state_vars : std::vector<std::string_view>()
    }
    for(var i : uint = 0; i < states.size(); i++) {
        tmpConv.state_vars.push(states.get(i).name);
    }
    tmpConv.convertJsNode(node);
    return builder.allocate_view(tmpConv.str.to_view());
}

func build_child_path(builder : *mut ASTBuilder, parentPath : std::string_view, childIndex : uint) : std::string_view {
    var p = std::string();
    if(parentPath.size() <= 2) {
        p.append_view("[");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    } else {
        p.append_view(parentPath.subview(0, parentPath.size() - 1));
        p.append_view(",");
        p.append_integer(childIndex as bigint);
        p.append_view("]");
    }
    return builder.allocate_view(p.to_view());
}

func render_universal_jsx(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    path : std::string_view,
    states : &std::vector<UniversalStateDecl>,
    textBindings : &mut std::vector<UniversalTextBinding>,
    eventBindings : &mut std::vector<UniversalEventBinding>,
    out : &mut std::string
) : bool {
    if(node == null) return false;

    switch(node.kind) {
        JsNodeKind.JSXText => {
            const text = node as *mut JsJSXText;
            escape_html_append(out, text.value);
            return true;
        }
        JsNodeKind.JSXExpressionContainer => {
            const container = node as *mut JsJSXExpressionContainer;
            if(container.expression != null) {
                if(container.expression.kind == JsNodeKind.Identifier) {
                    const id = container.expression as *mut JsIdentifier;
                    if(has_state(states, id.value)) {
                        const initial = find_state_init_text(states, id.value);
                        out.append_view("<span>");
                        escape_html_append(out, initial);
                        out.append_view("</span>");
                        textBindings.push(UniversalTextBinding {
                            stateName : id.value,
                            path : path
                        });
                        return true;
                    }
                } else if(container.expression.kind == JsNodeKind.Literal) {
                    const lit = container.expression as *mut JsLiteral;
                    escape_html_append(out, strip_js_string_quotes(lit.value));
                    return true;
                } else if(container.expression.kind == JsNodeKind.ObjectLiteral) {
                    const obj = container.expression as *mut JsObjectLiteral;
                    if(obj.properties.size() == 1) {
                        const prop = obj.properties.get(0);
                        if(prop.value != null && prop.value.kind == JsNodeKind.Identifier) {
                            const id = prop.value as *mut JsIdentifier;
                            if(has_state(states, id.value)) {
                                const initial = find_state_init_text(states, id.value);
                                out.append_view("<span>");
                                escape_html_append(out, initial);
                                out.append_view("</span>");
                                textBindings.push(UniversalTextBinding {
                                    stateName : id.value,
                                    path : path
                                });
                                return true;
                            }
                        }
                    }
                }
            }
            // Any unsupported JSX expression should fall back to runtime rendering.
            return false;
        }
        JsNodeKind.JSXFragment => {
            const frag = node as *mut JsJSXFragment;
            var childElementIndex : uint = 0;
            for(var i : uint = 0; i < frag.children.size(); i++) {
                const child = frag.children.get(i);
                if(child == null) continue;
                if(child.kind == JsNodeKind.JSXElement) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, out)) {
                        return false;
                    }
                    childElementIndex++;
                } else if(child.kind == JsNodeKind.JSXExpressionContainer) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, out)) {
                        return false;
                    }
                    const cont = child as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.Identifier) {
                        const id = cont.expression as *mut JsIdentifier;
                        if(has_state(states, id.value)) childElementIndex++;
                    }
                } else {
                    if(!render_universal_jsx(builder, child, path, states, textBindings, eventBindings, out)) {
                        return false;
                    }
                }
            }
            return true;
        }
        JsNodeKind.JSXElement => {
            const element = node as *mut JsJSXElement;
            if(element.opening.tagName == null || element.opening.tagName.kind != JsNodeKind.Identifier) return false;
            const tagNode = element.opening.tagName as *mut JsIdentifier;
            const tagName = tagNode.value;
            if(!is_native_tag(tagName)) return false;

            out.append('<');
            out.append_view(tagName);

            var classes = std::string();
            var attributesToEmit = std::vector<*mut JsJSXAttribute>();

            for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
                const attrNode = element.opening.attributes.get(i);
                if(attrNode == null || attrNode.kind != JsNodeKind.JSXAttribute) continue;
                const attr = attrNode as *mut JsJSXAttribute;
                
                if(is_event_attr_name(attr.name)) {
                    if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                        const container = attr.value as *mut JsJSXExpressionContainer;
                        const handler = js_node_to_source(builder, container.expression, states);
                        eventBindings.push(UniversalEventBinding {
                            eventName : event_attr_to_dom_event(builder, attr.name),
                            path : path,
                            handlerExpr : handler
                        });
                    }
                    continue;
                }

                if(attr.name.equals("class")) {
                    if(attr.value != null) {
                        if(!classes.empty()) classes.append(' ');
                        if(attr.value.kind == JsNodeKind.Literal) {
                            classes.append_view(strip_js_string_quotes((attr.value as *mut JsLiteral).value));
                        } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                            const container = attr.value as *mut JsJSXExpressionContainer;
                            if(container.expression != null && container.expression.kind == JsNodeKind.Literal) {
                                classes.append_view(strip_js_string_quotes((container.expression as *mut JsLiteral).value));
                            } else if(container.expression != null && container.expression.kind == JsNodeKind.Identifier) {
                                const id = container.expression as *mut JsIdentifier;
                                if(has_state(states, id.value)) {
                                    classes.append_view(find_state_init_text(states, id.value));
                                } else { return false; }
                            } else { return false; }
                        } else { return false; }
                    }
                } else {
                    attributesToEmit.push(attr);
                }
            }

            if(!classes.empty()) {
                out.append_view(" class=\"");
                escape_html_append(out, classes.to_view());
                out.append('"');
            }

            for(var i : uint = 0; i < attributesToEmit.size(); i++) {
                const attr = attributesToEmit.get(i);
                out.append(' ');
                out.append_view(attr.name);
                if(attr.value != null) {
                    out.append_view("=\"");
                    if(attr.value.kind == JsNodeKind.Literal) {
                        const lit = attr.value as *mut JsLiteral;
                        escape_html_append(out, strip_js_string_quotes(lit.value));
                    } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                        const container = attr.value as *mut JsJSXExpressionContainer;
                        if(container.expression != null && container.expression.kind == JsNodeKind.Identifier) {
                             const id = container.expression as *mut JsIdentifier;
                             if(has_state(states, id.value)) {
                                 escape_html_append(out, find_state_init_text(states, id.value));
                             } else { return false; }
                        } else if(container.expression != null && container.expression.kind == JsNodeKind.Literal) {
                             const lit = container.expression as *mut JsLiteral;
                             escape_html_append(out, strip_js_string_quotes(lit.value));
                        } else { return false; }
                    }
                    out.append('"');
                }
            }
            out.append('>');

            var childElementIndex : uint = 0;
            for(var i : uint = 0; i < element.children.size(); i++) {
                const child = element.children.get(i);
                if(child == null) continue;
                if(child.kind == JsNodeKind.JSXElement) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, out)) return false;
                    childElementIndex++;
                } else if(child.kind == JsNodeKind.JSXExpressionContainer) {
                    const childPath = build_child_path(builder, path, childElementIndex);
                    if(!render_universal_jsx(builder, child, childPath, states, textBindings, eventBindings, out)) return false;
                    const cont = child as *mut JsJSXExpressionContainer;
                    if(cont.expression != null && cont.expression.kind == JsNodeKind.Identifier) {
                        const id = cont.expression as *mut JsIdentifier;
                        if(has_state(states, id.value)) childElementIndex++;
                    }
                } else {
                    if(!render_universal_jsx(builder, child, path, states, textBindings, eventBindings, out)) return false;
                }
            }

            out.append_view("</");
            out.append_view(tagName);
            out.append('>');
            return true;
        }
        default => {
            return false;
        }
    }
    return false;
}

func find_returned_jsx(block : *mut JsBlock) : *mut JsNode {
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i);
        if(stmt != null && stmt.kind == JsNodeKind.Return) {
            const ret = stmt as *mut JsReturn;
            if(ret.value != null && (ret.value.kind == JsNodeKind.JSXElement || ret.value.kind == JsNodeKind.JSXFragment)) {
                return ret.value;
            }
        }
    }
    return null;
}

func collect_states(builder : *mut ASTBuilder, block : *mut JsBlock, outStates : &mut std::vector<UniversalStateDecl>) {
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i);
        if(stmt == null || stmt.kind != JsNodeKind.VarDecl) continue;
        const decl = stmt as *mut JsVarDecl;
        if(!decl.keyword.equals(view("state")) || decl.name.empty()) continue;
        var initExpr = view("undefined");
        var initText = view("");
        if(decl.value != null) {
            initExpr = js_node_to_source(builder, decl.value, outStates);
            if(decl.value.kind == JsNodeKind.Literal) {
                const lit = decl.value as *mut JsLiteral;
                initText = builder.allocate_view(strip_js_string_quotes(lit.value));
            } else {
                initText = initExpr;
            }
        }
        outStates.push(UniversalStateDecl {
            name : decl.name,
            initExpr : initExpr,
            initText : initText
        });
    }
}

func compute_universal_template(builder : *mut ASTBuilder, comp : *mut JsComponentDecl) {
    if(comp == null || comp.body == null || comp.body.kind != JsNodeKind.Block) return;
    const block = comp.body as *mut JsBlock;

    var states = std::vector<UniversalStateDecl>();
    collect_states(builder, block, states);

    const returned = find_returned_jsx(block);
    if(returned == null) return;

    var textBindings = std::vector<UniversalTextBinding>();
    var eventBindings = std::vector<UniversalEventBinding>();
    var html = std::string();

    if(!render_universal_jsx(builder, returned, view("[]"), states, textBindings, eventBindings, html)) {
        return;
    }

    var init = std::string();
    for(var i : uint = 0; i < states.size(); i++) {
        const st = states.get(i);
        init.append_view("const ");
        init.append_view(st.name);
        init.append_view(" = $_us(");
        init.append_view(st.initExpr);
        init.append_view(");");
    }
    for(var i : uint = 0; i < textBindings.size(); i++) {
        const b = textBindings.get(i);
        init.append_view(b.stateName);
        init.append_view(".subscribe(v=>$_ut(root,");
        init.append_view(b.path);
        init.append_view(").textContent=v);");
    }
    for(var i : uint = 0; i < eventBindings.size(); i++) {
        const e = eventBindings.get(i);
        init.append_view("$_ut(root,");
        init.append_view(e.path);
        init.append_view(").addEventListener('");
        init.append_view(e.eventName);
        init.append_view("',");
        init.append_view(e.handlerExpr);
        init.append_view(");");
    }

    comp.signature.universalTemplate = builder.allocate_view(html.to_view());
    comp.signature.universalInit = builder.allocate_view(init.to_view());
}

func append_escaped_single_quoted(out : &mut std::string, value : std::string_view) {
    for(var i : uint = 0; i < value.size(); i++) {
        const c = value.get(i);
        switch(c) {
            '\\' => out.append_view("\\\\")
            '\'' => out.append_view("\\'")
            '\n' => out.append_view("\\n")
            '\r' => out.append_view("\\r")
            '\t' => out.append_view("\\t")
            default => out.append(c)
        }
    }
}

func fix_support_page_node(
    support : &mut SymResSupport,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendHeadJsCharFn = page.child("append_js_char")
    if(appendHeadJsCharFn == null) {
        return false;
    }

    const appendHeadJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendHeadJsCharPtrFn == null) {
        return false;
    }

    const appendHeadJsFn = page.child("append_js");
    if(appendHeadJsFn == null) {
        return false;
    }

    const appendHeadJsIntFn = page.child("append_js_integer");
    if(appendHeadJsIntFn == null) {
        return false;
    }

    const appendHeadJsUIntFn = page.child("append_js_uinteger");
    if(appendHeadJsUIntFn == null) {
        return false;
    }

    const appendHeadJsFloatFn = page.child("append_js_float");
    if(appendHeadJsFloatFn == null) {
        return false;
    }

    const appendHeadJsDoubleFn = page.child("append_js_double");
    if(appendHeadJsDoubleFn == null) {
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) return false;
    const appendHtmlCharFn = page.child("append_html_char");
    if(appendHtmlCharFn == null) return false;
    const appendHtmlCharPtrFn = page.child("append_html_char_ptr");
    if(appendHtmlCharPtrFn == null) return false;
    const appendHtmlIntFn = page.child("append_html_integer");
    if(appendHtmlIntFn == null) return false;
    const appendHtmlUIntFn = page.child("append_html_uinteger");
    if(appendHtmlUIntFn == null) return false;
    const appendHtmlFloatFn = page.child("append_html_float");
    if(appendHtmlFloatFn == null) return false;
    const appendHtmlDoubleFn = page.child("append_html_double");
    if(appendHtmlDoubleFn == null) return false;
    // Optional: append_html_attributes_spread may not exist yet
    const appendHtmlAttributesSpreadFn = page.child("append_html_attributes_spread");

    const requireComponentFn = page.child("require_component");
    if(requireComponentFn == null) {
        return false;
    }

    const setComponentHashFn = page.child("set_component_hash");
    if(setComponentHashFn == null) {
        return false;
    }

    support.requireComponentFn = requireComponentFn;
    support.setComponentHashFn = setComponentHashFn;

    support.appendHeadJsFn = appendHeadJsFn
    support.appendHeadJsCharFn = appendHeadJsCharFn
    support.appendHeadJsCharPtrFn = appendHeadJsCharPtrFn
    support.appendHeadJsIntFn = appendHeadJsIntFn;
    support.appendHeadJsUIntFn = appendHeadJsUIntFn;
    support.appendHeadJsFloatFn = appendHeadJsFloatFn;
    support.appendHeadJsDoubleFn = appendHeadJsDoubleFn;

    support.appendHtmlFn = appendHtmlFn;
    support.appendHtmlCharFn = appendHtmlCharFn;
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn;
    support.appendHtmlIntFn = appendHtmlIntFn;
    support.appendHtmlUIntFn = appendHtmlUIntFn;
    support.appendHtmlFloatFn = appendHtmlFloatFn;
    support.appendHtmlDoubleFn = appendHtmlDoubleFn;
    support.appendHtmlAttributesSpreadFn = appendHtmlAttributesSpreadFn;

    return true
}

@no_mangle
public func universal_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode as *mut ASTNode;
}

@no_mangle
public func universal_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const body = root.signature.functionNode.add_body();
    
    var support = root.support

    var converter = JsConverter {
        builder : builder,
        support : &mut support,
        vec : body,
        parent : root.signature.functionNode as *mut ASTNode,
        str : std::string(),
        jsx_parent : view(""), 
        t_counter : 0,
        state_vars : std::vector<std::string_view>()
    }
    
    const location = intrinsics::get_raw_location()
    
    var emitted = std::vector<size_t>();
    
    for(var i : uint = 0; i < root.components.size(); i++) {
        const element = root.components.get(i);
        if(element.componentSignature != null) {
            const signature = element.componentSignature;
            const hash = signature.functionNode.getEncodedLocation() as size_t;
            
            var already_emitted = false;
            for(var j : uint = 0; j < emitted.size(); j++) {
                if(emitted.get(j) == hash) {
                    already_emitted = true;
                    break;
                }
            }
            
            if(!already_emitted) {
                emitted.push(hash);
                
                converter.put_chain_in()
                
                var requireCall = converter.make_require_component_call(hash)
                var ifStmt = builder.make_if_stmt(requireCall as *mut Value, converter.parent, location)
                var thenBody = ifStmt.get_body()
                
                thenBody.push(converter.make_set_component_hash_call(hash))
                
                var base = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
                var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
                var call = builder.make_function_call_node(base as *mut ChainValue, converter.parent, location)
                call.get_args().push(pageId as *mut Value)
                thenBody.push(call as *mut ASTNode)
                
                converter.vec.push(ifStmt as *mut ASTNode)
            }
        }
    }

    if(root.body != null && root.body.kind == JsNodeKind.Block) {
        const block = root.body as *mut JsBlock;
        const returned = find_returned_jsx(block);
        if(returned != null) {
            // 1. Perform SSR (emit to pageHtml buffer)
            converter.target = BufferType.HTML;
            converter.convertJsNode(returned);
            converter.put_chain_in();
            
            // 2. Perform hydration script emission (emit to pageJs buffer)
            // The JsJSXComponent conversion inside convertJsNode(returned) already 
            // emits hydration for child components.
            // We just need to make sure the ROOT of THIS component also hydrates if needed.
            // Actually, the Component itself is a target of hydration in the parent.
        }
    }

    // Still need to emit the JS function definition for this component
    // so it can be used for hydration from other places.
    converter.target = BufferType.JavaScript;
    converter.str.append_view("function ");
    get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
    converter.str.append_view("(");
    converter.str.append_view(root.signature.propsName);
    converter.str.append_view("){");
    
    if(!root.signature.universalTemplate.empty()) {
        converter.str.append_view("const tpl=document.createElement('template');tpl.innerHTML='");
        append_escaped_single_quoted(converter.str, root.signature.universalTemplate);
        converter.str.append_view("';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);");
        get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
        converter.str.append_view(".__hydrate(n,");
        converter.str.append_view(root.signature.propsName);
        converter.str.append_view("||{});return n;}");
    } else {
        if(root.body != null) {
            converter.convertJsNode(root.body);
        } else {
             converter.str.append_view("return document.createTextNode('');}");
        }
    }

    get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
    converter.str.append_view(".__hydrate=(root,");
    converter.str.append_view(root.signature.propsName);
    converter.str.append_view(")=>{");
    converter.str.append_view(root.signature.universalInit);
    converter.str.append_view("};");
    
    converter.str.append_view("if(window.$_ureg)window.$_ureg('");
    get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
    converter.str.append_view("',");
    get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str);
    converter.str.append_view(");");

    converter.put_chain_in();
    
    return root.signature.functionNode as *mut ASTNode;
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

public func cross_mod_sym_decl_proxy_fn(obj : *mut void, node : *mut EmbeddedNode, fn : CrossModuleSymbolDeclarerFn, at_least_spec : AccessSpecifier) {
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    if (comp.signature.access == AccessSpecifier.Public) {
        fn(obj, comp.signature.name, node)
    }
}

@no_mangle
public func universal_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    
    const location = parser.getEncodedLocation(parser.getToken());
    
    // Parse Identifier
    var name = std::string_view();
    if(parser.getToken().type == JsTokenType.Identifier as int) {
        name = builder.allocate_view(parser.getToken().value);
        parser.increment();
    } else {
        parser.error("expected component name");
        return null;
    }

    if(!parser.increment_if(JsTokenType.LParen as int)) {
        parser.error("expected (");
    }

    var propsName = std::string_view();
    if(parser.getToken().type == JsTokenType.Identifier as int) {
        propsName = builder.allocate_view(parser.getToken().value);
        parser.increment();
    } else {
        parser.error("expected identifier for props");
    }

    var params = std::vector<ComponentParam>();
    if(parser.increment_if(JsTokenType.Colon as int)) {        
        while(true) {
            const t = parser.getToken();
            if(t.type == JsTokenType.Identifier as int) {
                    var paramName = builder.allocate_view(t.value);
                    parser.increment();
                    
                    var is_optional = false;
                    if(parser.getToken().type == JsTokenType.Question as int) {
                        is_optional = true;
                        parser.increment();
                    }
                    
                    params.push(ComponentParam { name : paramName, is_optional : is_optional });
            } else {
                    parser.error("expected identifier param");
                    break;
            }
            if(parser.getToken().type == JsTokenType.Comma as int) {
                    parser.increment();
            } else {
                    break;
            }
        }
    }

    if(!parser.increment_if(JsTokenType.RParen as int)) {
        parser.error("expected )");
    }

    var comp = builder.allocate<JsComponentDecl>()
    new (comp) JsComponentDecl {
        base : JsNode { kind : JsNodeKind.ComponentDecl },
        signature : ComponentSignature {
            name : name,
            propsName : propsName,
            params : params,
            functionNode : null,
            mountStrategy : MountStrategy.Universal, // Important: Set mount strategy
            access : spec
        },
        body : null,
        support : SymResSupport {}, 
        dyn_values : std::vector<*mut Value>(),
        components : std::vector<*mut JsJSXElement>(),
        htmlPageNode : null
    }

    var jsParser = JsParser {
        dyn_values : &mut comp.dyn_values,
        components : &mut comp.components
    }
    
    if(parser.getToken().type == JsTokenType.LBrace as int) {
        var body = jsParser.parseBlock(parser, builder);
        comp.body = body;
        compute_universal_template(builder, comp);
        
        const nodes_arr : []*mut ASTNode = []
        
        const node = builder.make_top_level_embedded_node(spec, std::string_view("universal"), comp, node_known_type_func, node_child_res_func, cross_mod_sym_decl_proxy_fn, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size()), parser.getParentNode(), location);

        const controller = parser.getAnnotationController();

        const definition = controller.getDefinition("component");
        if(definition == null) {
            parser.error("component annotation is not defined")
            return node
        }

        const args : []*mut Value = []

        controller.mark(node, definition, std::span<*mut Value>(args));

        return node;
    } else {
        parser.error("expected {");
        return null;
    }
}

@no_mangle
public func universal_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer { }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
