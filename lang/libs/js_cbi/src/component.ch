@no_mangle
public func component_symResSigNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    // this is where signature should be resolved
    // like signature of function
    // declare the node here as well so others 
}

@no_mangle
public func component_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsComponentDecl;

    root.htmlPageNode = resolver.find(std::string_view("HtmlPage"));
    if(root.htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    sym_res_components(root.components, resolver, loc)
}

@no_mangle
public func component_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = intrinsics::get_raw_location();
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}

func fix_support_page_node(
    support : &mut SymResSupport,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendJsCharFn = page.child("append_js_char")
    if(appendJsCharFn == null) {
        return false;
    }

    const appendJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendJsCharPtrFn == null) {
        return false;
    }

    const appendJsFn = page.child("append_js");
    if(appendJsFn == null) {
        return false;
    }

    const appendJsIntFn = page.child("append_js_integer");
    if(appendJsIntFn == null) {
        return false;
    }

    const appendJsUIntFn = page.child("append_js_uinteger");
    if(appendJsUIntFn == null) {
        return false;
    }

    const appendJsFloatFn = page.child("append_js_float");
    if(appendJsFloatFn == null) {
        return false;
    }

    const appendJsDoubleFn = page.child("append_js_double");
    if(appendJsDoubleFn == null) {
        return false;
    }

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

    support.appendJsCharFn = appendJsCharFn
    support.appendJsCharPtrFn = appendJsCharPtrFn
    support.appendJsFn = appendJsFn
    support.appendJsIntFn = appendJsIntFn;
    support.appendJsUIntFn = appendJsUIntFn;
    support.appendJsFloatFn = appendJsFloatFn;
    support.appendJsDoubleFn = appendJsDoubleFn;

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
public func component_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
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
public func component_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut JsComponentDecl;
    const body = root.signature.functionNode.add_body();
    
    // override support functions with their head_js counterparts
    // so the converter writes to pageHeadJs instead of pageJs
    var support = root.support
    support.appendJsFn = support.appendHeadJsFn
    support.appendJsCharFn = support.appendHeadJsCharFn
    support.appendJsCharPtrFn = support.appendHeadJsCharPtrFn
    support.appendJsIntFn = support.appendHeadJsIntFn
    support.appendJsUIntFn = support.appendHeadJsUIntFn
    support.appendJsFloatFn = support.appendHeadJsFloatFn
    support.appendJsDoubleFn = support.appendHeadJsDoubleFn

    var converter = JsConverter {
        builder : builder,
        support : &mut support,
        vec : body,
        parent : root.signature.functionNode as *mut ASTNode,
        str : std::string(),
        jsx_parent : view("$c_root"),
        t_counter : 0
    }
    
    // function $c_Greeting($c_props) {
    converter.str.append_view("function $c_")
    converter.str.append_view(root.signature.name)
    converter.str.append_view("($c_props) {")
    
    // const { name, ... } = $c_props
    if(!root.signature.params.empty()) {
        converter.str.append_view("const { ")
        for(var i : uint = 0; i < root.signature.params.size(); i++) {
            if(i > 0) converter.str.append_view(", ")
            converter.str.append_view(root.signature.params.get(i).name)
        }
        converter.str.append_view(" } = $c_props;")
    }
    
    // const $c_root = document.createDocumentFragment();
    converter.str.append_view("const $c_root = document.createDocumentFragment();")
    
    if(root.body != null) {
        converter.convertJsNode(root.body);
    }
    
    // return $c_root
    converter.str.append_view("return $c_root; }")
    
    converter.put_chain_in();
    
    return root.signature.functionNode as *mut ASTNode;
}

@no_mangle
public func component_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    // #component Greeting(params) { body }
    // Token #component is already consumed ? No, this is called as callback for #component macro.
    // The parser is AFTER #component.
    
    // Parse Identifier
    var name = std::string_view();
    if(parser.getToken().type == JsTokenType.Identifier as int) {
        name = builder.allocate_view(parser.getToken().value);
        parser.increment();
    } else {
        parser.error("expected component name");
        return null; // Handle error gracefully if possible
    }

    if(!parser.increment_if(JsTokenType.LParen as int)) {
        parser.error("expected (");
    }

    var params = std::vector<ComponentParam>();
    if(parser.getToken().type != JsTokenType.RParen as int) {
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

    // Initialize Component definition first to handle dynamic values
    var comp = builder.allocate<JsComponentDecl>()
    new (comp) JsComponentDecl {
        base : JsNode { kind : JsNodeKind.ComponentDecl },
        signature : ComponentSignature {
            name : name,
            params : params,
            functionNode : null
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
        
        const node = builder.make_embedded_node(std::string_view("component"), comp, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size()), null, intrinsics::get_raw_location());

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
public func component_initializeLexer(lexer : *mut Lexer) {
    js_initializeLexer(lexer);
}