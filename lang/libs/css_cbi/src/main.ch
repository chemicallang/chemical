@no_mangle
public func css_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation()
    const root = node.getDataPtr() as *mut CSSOM;
    sym_res_root(root, resolver, loc)
}

@no_mangle
public func css_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = ASTConverter {
        builder : builder,
        support : &mut root.support,
        vec : scope_nodes,
        parent : root.parent
        str : std::string()
    }
    converter.convertCSSOM(root);
    return scope;
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

@no_mangle
public func css_symResValue(resolver : *mut SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation();
    const root = value.getDataPtr() as *mut CSSOM;
    sym_res_root(root, resolver, loc)
    return true;
}

@no_mangle
public func css_replacementValue(builder : *mut ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    var converter = ASTConverter {
        builder : builder,
        support : &mut root.support,
        vec : scope_nodes,
        parent : root.parent
        str : std::string()
    }
    converter.convertCSSOM(root);
    // const view2 = builder.allocate_view(converter.str.to_view())
    const classNameVal = builder.make_string_value(root.className, loc)
    block_val.setCalculatedValue(classNameVal)
    return block_val;
}

@no_mangle
public func css_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const loc = intrinsics::get_raw_location();
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
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(std::string_view("css"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the css macro");
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
        where : CSSLexerWhere.Declaration
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}