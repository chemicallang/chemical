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
            jsEmitFunctionNode : null,
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
