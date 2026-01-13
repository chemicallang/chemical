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
    sym_res_components(root.components, resolver, loc)
}

@no_mangle
public func component_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = intrinsics::get_raw_location();
    const comp = node.getDataPtr() as *mut JsComponentDecl;
    resolver.declare(comp.signature.name, node);
}

@no_mangle
public func component_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    return null
}

@no_mangle
public func component_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    return null
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
            params : params
        },
        body : null,
        support : SymResSupport {}, 
        dyn_values : std::vector<*mut Value>(),
        components : std::vector<*mut JsJSXElement>(),
    }

    var jsParser = JsParser {
        dyn_values : &mut comp.dyn_values,
        components : &mut comp.components
    }
    
    if(parser.getToken().type == JsTokenType.LBrace as int) {
        var body = jsParser.parseBlock(parser, builder);
        comp.body = body;
        
        const nodes_arr : []*mut ASTNode = []
        
        const node = builder.make_embedded_node(std::string_view("js_component"), comp, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size()), null, intrinsics::get_raw_location());

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