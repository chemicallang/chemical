public func node_symbol_resolve_func(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation()
    const root = node.getDataPtr() as *mut CSSOM;
    sym_res_root(root, resolver, loc)
}

public func node_replacement_func(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = ASTConverter {
        builder : builder,
        support : &root.support,
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

public func node_traversal_func(node : *EmbeddedNode, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = node.getDataPtr() as *mut CSSOM;
    traverse_cssom(root, data, traverse)
}

public func value_symbol_resolve_func(resolver : *SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation();
    const root = value.getDataPtr() as *mut CSSOM;
    sym_res_root(root, resolver, loc)
}

public func value_replacement_func(builder : *ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut CSSOM;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    var converter = ASTConverter {
        builder : builder,
        support : &root.support,
        vec : scope_nodes,
        parent : root.parent
        str : std::string()
    }
    converter.convertCSSOM(root);
    const view = builder.allocate_view(converter.str.to_view())
    const classNameVal = builder.make_string_value(root.className, loc)
    block_val.setCalculatedValue(classNameVal)
    return block_val;
}

public func value_traversal_func(value : *EmbeddedValue, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = value.getDataPtr() as *mut CSSOM;
    traverse_cssom(root, data, traverse)
}

@no_mangle
public func css_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseCSSOM(parser, builder);
        const type = builder.make_string_type(loc)
        const value = builder.make_embedded_value(root, type, value_symbol_resolve_func, value_replacement_func, value_traversal_func, loc);
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
        const node = builder.make_embedded_node(root, node_symbol_resolve_func, node_replacement_func, node_known_type_func, node_child_res_func, node_traversal_func, root.parent, loc);
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
                css.lb_count--;
                if(css.lb_count == 1) {
                    css.other_mode = false;
                    css.chemical_mode = false;
                }
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
        where : CSSLexerWhere.Declaration
    }
    lexer.setUserLexer(ptr, getNextToken)
}