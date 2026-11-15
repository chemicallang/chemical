public unsafe func node_known_type_func(value : *EmbeddedNode) : *BaseType {
        unsafe {
        return null;

        }
    }

public unsafe func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
        unsafe {
        return null;

        }
    }

public unsafe func cross_mod_sym_decl_proxy_fn(obj : *mut void, node : *mut EmbeddedNode, fn : CrossModuleSymbolDeclarerFn, at_least_spec : AccessSpecifier) {
        unsafe {
        const comp = node.getDataPtr() as *mut JsComponentDecl;
        if (comp.signature.access == AccessSpecifier.Public) {
            fn(obj, &comp.signature.name, node)
        }

        }
    }

// A props annotation beginning with an uppercase letter names a type
// (`props : CardProps`); a lowercase one names a required prop
// (`props : title`), the pre-typed form. Keeping the two distinct by case makes
// the syntax unambiguous without a new keyword.
func starts_uppercase(view : std::string_view) : bool {
    if(view.size() == 0) return false
    const c = view.at(0)
    return c >= 'A' && c <= 'Z'
}

@no_mangle
public unsafe func universal_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
        unsafe {

        const location = parser.getEncodedLocation(parser.getToken());

        // Parse Identifier
        var name = std::string_view();
        if(parser.getToken().type == JsTokenType.Identifier as int) {
            name = builder.allocate_view(&parser.getToken().value);
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
            propsName = builder.allocate_view(&parser.getToken().value);
            parser.increment();
        } else {
            parser.error("expected identifier for props");
        }

        var params = std::vector<ComponentParam>();
        var propsTypeName = std::string_view();
        if(parser.increment_if(JsTokenType.Colon as int)) {
            // Two forms share the colon slot:
            //   props : CardProps      -> typed props (a struct; uppercase initial)
            //   props : title, tag?    -> required prop names (legacy; lowercase)
            // The case rule makes the two unambiguous without a new keyword: a
            // type name is capitalised, a prop name is not.
            var names = std::vector<std::string_view>();
            var anyOptional = false;
            while(true) {
                const t = parser.getToken();
                if(t.type != JsTokenType.Identifier as int) {
                    parser.error("expected identifier after ':'");
                    break;
                }
                names.push(builder.allocate_view(&t.value));
                parser.increment();
                if(parser.getToken().type == JsTokenType.Question as int) {
                    anyOptional = true;
                    parser.increment();
                }
                if(parser.getToken().type == JsTokenType.Comma as int) {
                    parser.increment();
                } else {
                    break;
                }
            }
            if(names.size() == 1 && !anyOptional && starts_uppercase(names.at(0))) {
                propsTypeName = names.at(0);
            } else {
                for(var i : uint = 0; i < names.size(); i++) {
                    params.push(ComponentParam { name : names.at(i), is_optional : anyOptional });
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
                propsTypeName : propsTypeName,
                functionNode : null,
                mountStrategy : MountStrategy.Universal, // Important: Set mount strategy
                access : spec,
                rootNodeCount : 0,
                className : std::string_view()
            },
            body : null,
            support : SymResSupport {},
            dyn_values : std::vector<*mut Value>(),
            components : std::vector<*mut JsJSXElement>(),
            htmlPageNode : null
        }

        var jsParser = JsParser {
            dyn_values : &raw mut comp.dyn_values,
            components : &raw mut comp.components,
            style_fn : universal_parse_style_block
        }

        if(parser.getToken().type == JsTokenType.LBrace as int) {
            var body = jsParser.parseBlock(parser, builder);
            comp.body = body;
            // Back-pointer for router validation (R11/R12): the parsed JS body.
            comp.signature.js_body = body as *mut void;

            const nodes_arr : []*mut ASTNode = []

            const node = builder.make_top_level_embedded_node(spec, std::string_view("universal"), comp, node_known_type_func, node_child_res_func, cross_mod_sym_decl_proxy_fn, unsafe(std::span<*mut ASTNode>(nodes_arr)), unsafe(std::span<*mut Value>(comp.dyn_values.data(), comp.dyn_values.size())), parser.getParentNode(), location);

            const controller = parser.getAnnotationController();

            const definition = controller.getDefinition("component");
            if(definition == null) {
                // parser.error("component annotation is not defined")
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
    }

@no_mangle
public unsafe func universal_initializeLexer(lexer : *mut Lexer) {
        unsafe {
        const file_allocator = lexer.getFileAllocator();
        const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
        new (ptr) JsLexer { }
        lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)

        }
    }
