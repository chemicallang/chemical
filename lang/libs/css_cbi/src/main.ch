@no_mangle
public func css_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    visitor.visitEmbeddedNode(node)
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation()
    const root = node.getDataPtr() as *mut CSSOM;
    sym_res_root(root, visitor, loc)
}

@no_mangle
public func css_replacementNode(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = ASTConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        // Statement position (`#css { ... }` on its own line): the macro's value
        // is the generated class and nothing can receive it, so the block is
        // emitted as a global stylesheet rather than dead class-scoped CSS.
        is_global_block : true,
        parent : root.parent,
        str : std::string()
    }
    // `#css` is always the page's own CSS. `#globalcss` / component `style { }`
    // leave `shared` set and may go into the shared assets sink instead.
    if(!root.shared) {
        const beginFn = converter.support.beginLocalCssFn
        scope_nodes.push(css_page_noarg_call(&mut converter, beginFn, std::string_view("begin_local_css"), loc) as *mut ASTNode)
    }
    converter.convertCSSOM(root, value.getEncodedLocation());
    if(!root.shared) {
        const endFn = converter.support.endLocalCssFn
        scope_nodes.push(css_page_noarg_call(&mut converter, endFn, std::string_view("end_local_css"), loc) as *mut ASTNode)
    }
    return scope;
}

// Builds a no-argument call on the page object: `page.<name>()`.
func css_page_noarg_call(converter : &mut ASTConverter, fnNode : *mut ASTNode, fnName : std::string_view, loc : ubigint) : *mut FunctionCallNode {
    const builder = converter.builder
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, loc)
    var id = builder.make_identifier(&fnName, fnNode, false, loc)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc)
    return builder.make_function_call_node(chain, converter.parent, loc)
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

@no_mangle
public func css_symResValue(visitor : *mut SymResLinkBody, value : *mut EmbeddedValue) : bool {
    visitor.visitEmbeddedValue(value)
    const resolver = visitor.getSymbolResolver();
    const loc = value.getEncodedLocation();
    const root = value.getDataPtr() as *mut CSSOM;
    sym_res_root(root, visitor, loc)
    return true;
}

@no_mangle
public func css_replacementValue(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    var converter = ASTConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    if(!root.shared) {
        const beginFn = converter.support.beginLocalCssFn
        scope_nodes.push(css_page_noarg_call(&mut converter, beginFn, std::string_view("begin_local_css"), loc) as *mut ASTNode)
    }
    converter.convertCSSOM(root, value.getEncodedLocation());
    if(!root.shared) {
        const endFn = converter.support.endLocalCssFn
        scope_nodes.push(css_page_noarg_call(&mut converter, endFn, std::string_view("end_local_css"), loc) as *mut ASTNode)
    }
    // const view2 = builder.allocate_view(converter.str.to_view())
    const classNameVal = builder.make_string_value(&root.className, loc)
    block_val.setCalculatedValue(classNameVal)
    return block_val;
}

@no_mangle
public func css_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        const type = builder.make_string_type(loc)
        const value = builder.make_embedded_value(std::string_view("css"), root, type, std::span<*mut ASTNode>(null, 0), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the css macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

@no_mangle
public func css_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("css"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the css macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

// `#globalcss { ... }` — app-wide global CSS. Unlike `#css` (always the page's
// own CSS), a globalcss block prefers the shared assets sink when the page has
// one attached, falling back to the page. It reuses the `css` embedded value
// with `shared = true`, so it shares the CSS emission and symres machinery.
@no_mangle
public func globalcss_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        root.shared = true
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("css"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the globalcss macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

public func getNextToken(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    if(css.other_mode) {
        if(css.chemical_mode) {
            var nested = lexer.getEmbeddedToken();
            if(nested.type == ChemicalTokenType.LBrace) {
                css.lb_count++;
            } else if(nested.type == ChemicalTokenType.RBrace) {
                if(css.lb_count == css.start_chemical_lb_count) {
                    css.other_mode = false;
                    css.chemical_mode = false;
                }
                css.lb_count--;
            }
            return nested;
        }
    }
    const t = getNextToken2(css, lexer);
    return t;
}

@no_mangle
public func css_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(CSSLexer), alignof(CSSLexer)) as *mut CSSLexer;
    new (ptr) CSSLexer {
        other_mode : false,
        chemical_mode : false,
        lb_count : 0,
        start_chemical_lb_count : 1,
        at_rule : false,
        where_state : CSSLexerWhere.Declaration,
        tokens_since_colon : 0,
        has_chemical_in_value : false
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}