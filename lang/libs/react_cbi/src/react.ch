@no_mangle
public func react_symResSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
    // no type or value is included in this node which requires resolution at link signature
    // other than HtmlPage type, how are we are comfortable resolving that after
}

@no_mangle
public func react_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsComponentDecl;

    root.htmlPageNode = resolver.find(std::string_view("HtmlPage"));
    if(root.htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    const builder = resolver.getJobBuilder();

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

    const ssrTextNode = resolver.find("SsrText")
    if(ssrTextNode != null) {
        root.support.ssrTextLinkedNode = ssrTextNode
    }

    const ssrAttrListNode = resolver.find("SsrAttributeList")
    if(ssrAttrListNode != null) {
        root.support.ssrAttributeListNode = ssrAttrListNode
    }

    root.signature.functionNode = funcDecl;

}

@no_mangle
public func react_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = intrinsics::get_raw_location();
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}

func fix_support_page_node(
    support : &mut SymResSupport,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendHeadJsCharFn = page.child("append_head_js_char")
    if(appendHeadJsCharFn == null) {
        return false;
    }

    const appendHeadJsCharPtrFn = page.child("append_head_js_char_ptr")
    if(appendHeadJsCharPtrFn == null) {
        return false;
    }

    const appendHeadJsFn = page.child("append_head_js");
    if(appendHeadJsFn == null) {
        return false;
    }

    const appendHeadJsIntFn = page.child("append_head_js_integer");
    if(appendHeadJsIntFn == null) {
        return false;
    }

    const appendHeadJsUIntFn = page.child("append_head_js_uinteger");
    if(appendHeadJsUIntFn == null) {
        return false;
    }

    const appendHeadJsFloatFn = page.child("append_head_js_float");
    if(appendHeadJsFloatFn == null) {
        return false;
    }

    const appendHeadJsDoubleFn = page.child("append_head_js_double");
    if(appendHeadJsDoubleFn == null) {
        return false;
    }

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

    return true
}

@no_mangle
public func react_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    return root.signature.functionNode as *mut ASTNode;
}

@no_mangle
public func react_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
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
        t_counter : 0
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
                
                var targetNode = signature.functionNode as *mut FunctionDeclaration;
                var targetName = signature.name;
                var base = builder.make_identifier(targetName, targetNode as *mut ASTNode, false, location)
                var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
                var call = builder.make_function_call_node(base, converter.parent, location)
                call.get_args().push(pageId as *mut Value)
                thenBody.push(call as *mut ASTNode)
                
                converter.vec.push(ifStmt as *mut ASTNode)
            }
        }
    }
    
    // Convert to React element returning function
    // function Component(props) { return $_r.createElement(...) }
    
    converter.str.append_view("function ")
    get_module_scoped_name(root.signature.functionNode as *mut ASTNode, root.signature.name, converter.str)
    converter.str.append_view("(")
    converter.str.append_view(root.signature.propsName)
    converter.str.append_view(") ")
    
    if(root.body != null) {
        converter.convertJsNode(root.body)
    }
    
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
public func react_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    
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
            mountStrategy : MountStrategy.React, // Important: Set mount strategy
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
        
        const nodes_arr : []*mut ASTNode = []
        
        const node = builder.make_top_level_embedded_node(spec, std::string_view("react"), comp, node_known_type_func, node_child_res_func, cross_mod_sym_decl_proxy_fn, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size()), parser.getParentNode(), location);

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
public func react_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer { }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
