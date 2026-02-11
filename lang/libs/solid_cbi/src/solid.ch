@no_mangle
public func solid_symResSigNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    // this is where signature should be resolved
    // like signature of function
    // declare the node here as well so others 
}

@no_mangle
public func solid_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsComponentDecl;

    root.htmlPageNode = resolver.find(std::string_view("HtmlPage"));
    if(root.htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    sym_res_components(root.components, resolver)
}

@no_mangle
public func solid_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
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
public func solid_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const loc = value.getEncodedLocation();

    // func name(page : &mut HtmlPage) : void
    const linked = builder.make_linked_type(std::string_view("HtmlPage"), root.htmlPageNode, loc);
    const ref = builder.make_reference_type(linked as *mut BaseType, loc);
    const param = builder.make_function_param(std::string_view("page"), ref as *mut BaseType, 0, null, false, null, loc);
    
    const voidType = builder.make_void_type(loc);
    const funcDecl = builder.make_function(root.signature.name, loc, voidType as *mut BaseType, false, true, null, loc);
    
    funcDecl.get_params().push(param);
    funcDecl.add_body();

    // fixing support
    root.support.pageNode = param as *mut ASTNode
    fix_support_page_node(root.support, root.support.pageNode, loc)

    root.signature.functionNode = funcDecl;
    return funcDecl as *mut ASTNode;
}

@no_mangle
public func solid_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const body = root.signature.functionNode.add_body();
    
    var support = root.support

    var converter = JsConverter {
        builder : builder,
        support : &mut support,
        vec : body,
        parent : root.signature.functionNode as *mut ASTNode,
        str : std::string(),
        jsx_parent : view(""), // Not used in Preact mode like this
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
                
                var base = builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location)
                var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
                var call = builder.make_function_call_node(base as *mut ChainValue, converter.parent, location)
                call.get_args().push(pageId as *mut Value)
                thenBody.push(call as *mut ASTNode)
                
                converter.vec.push(ifStmt as *mut ASTNode)
            }
        }
    }
    
    // Convert to Solid component function
    // function Component(props) { return ... }
    
    converter.str.append_view("function ")
    converter.str.append_view(root.signature.name)
    converter.str.append_view("(")
    converter.str.append_view(root.signature.propsName)
    converter.str.append_view(") { return ")
    
    if(root.body != null) {
        // Here we expect root.body (Block) to contain statements.
        // But Solid components usually return a function or element.
        // If the user wrote `{ <div>...</div> }`, it's a block with expression statements.
        // If they wrote `{ var x = 1; return <div>{x}</div> }`, we need to handle that.
        // Standard component converter handles statements.
        // We should just convert the block content. But last statement should be return?
        // Or we wrap it?
        // If we strictly follow "Function Component", user writes:
        // #solid Func(props) { return <div/> }
        // We just output the body logic.
        
        // Wait, standard JS `component` macro logic above puts `return $c_root` at end.
        // For Solid, we don't return `root`. We return whatever the user code returns.
        // So we should NOT append "return ...".
        // Instead, we trust the user code to return something, OR if they just put an expression, we return it?
        // JS functions need explicit return unless arrow implicit return.
        // `#solid` uses `{ ... }` block. So explicit return is expected if typical JS.
        // However, if we want to support `#solid Comp { <div/> }` (implicit return), we'd need to check block.
        // Let's assume standard JS function body semantics for now: User must `return`.
        // But wait, line 189 was: `converter.str.append_view(") { return ")`
        // If I say `return`, then `convertJsNode(root.body)` (which outputs `{ ... }`) -> syntax error: `return { ... }`.
        
        // Correct logic:
        // `function Name(props) `
        // `convertJsNode(root.body)` (This handles the block braces if Block node is passed?)
        // `JsBlock` conversion usually outputs `{ ... }`.
        
        // Let's check `convertJsNode` for Block.
        // It outputs `{ ... }`.
        
        // So:
        converter.str = std::string() // reset from above append_view attempt which was wrong
        converter.put_chain_in() // clear buffer if any
        
        converter.str.append_view("function ")
        converter.str.append_view(root.signature.name)
        converter.str.append_view("(")
        converter.str.append_view(root.signature.propsName)
        converter.str.append_view(") ")
        
        converter.convertJsNode(root.body)
        
        // No auto return for now. User must provide `return`.
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

@no_mangle
public func solid_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    
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
            mountStrategy : MountStrategy.Solid // Important: Set mount strategy
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
        
        const node = builder.make_embedded_node(std::string_view("solid"), comp, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size()), null, location);

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
public func solid_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer {
        lb_count : 0,
        chemical_mode : false,
        jsx_depth : 0,
        in_jsx_tag : 0,
        jsx_brace_count : 0,
        tag_mode_stack : 0
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
