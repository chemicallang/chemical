@no_mangle
public func component_symResSigNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    // this is where signature should be resolved
    // like signature of function
    // declare the node here as well so others 
}

@no_mangle
public func component_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    // resolve body of the node
}

@no_mangle
public func component_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    // resolve body of the node
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

    var params = std::vector<std::string_view>();
    if(parser.getToken().type != JsTokenType.RParen as int) {
        while(true) {
            const t = parser.getToken();
            if(t.type == JsTokenType.Identifier as int) {
                 params.push(builder.allocate_view(t.value));
                 parser.increment();
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

    // Initialize Root first to handle dynamic values
    var root = builder.allocate<JsRoot>()
    new (root) JsRoot {
         statements : std::vector<*mut JsNode>(),
         parent : null, 
         support : SymResSupport {}, 
         dyn_values : std::vector<*mut Value>()
    }

    var jsParser = JsParser {
        dyn_values : &mut root.dyn_values
    }
    
    if(parser.getToken().type == JsTokenType.LBrace as int) {
        var body = jsParser.parseBlock(parser, builder);
        var comp = builder.allocate<JsComponentDecl>()
        new (comp) JsComponentDecl {
            base : JsNode { kind : JsNodeKind.ComponentDecl },
            name : name,
            params : params,
            body : body
        }
        
        root.statements.push(comp as *mut JsNode);
        
        const nodes_arr : []*mut ASTNode = []
        
        const node = builder.make_embedded_node(std::string_view("js_component"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), null, intrinsics::get_raw_location());
        
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