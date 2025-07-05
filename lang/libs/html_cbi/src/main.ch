public func node_symbol_resolve_func(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut HtmlRoot;
    sym_res_root(root, resolver, loc)
}

public func node_replacement_func(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut HtmlRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = ASTConverter {
        builder : builder,
        support : &root.support,
        vec : scope_nodes,
        parent : root.parent
        str : std::string()
    }
    converter.convertHtmlRoot(root);
    return scope;
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

public func node_traversal_func(node : *EmbeddedNode, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = node.getDataPtr() as *mut HtmlRoot;
    traverse_root(root, data, traverse)
}

public func value_symbol_resolve_func(resolver : *SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut HtmlRoot;
    sym_res_root(root, resolver, loc)
}

public func value_replacement_func(builder : *ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut HtmlRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    var converter = ASTConverter {
        builder : builder,
        support : &root.support,
        vec : scope_nodes,
        parent : root.parent
        str : std::string()
    }
    converter.convertHtmlRoot(root);
    const view = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(view, loc)
    block_val.setCalculatedValue(strValue)
    return block_val;
}

public func value_type_creation_func(builder : *ASTBuilder, value : *EmbeddedValue) : *BaseType {
    const loc = intrinsics::get_raw_location();
    return builder.make_string_type(loc);
}

public func value_traversal_func(value : *EmbeddedValue, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = value.getDataPtr() as *mut HtmlRoot;
    traverse_root(root, data, traverse)
}

@no_mangle
public func html_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    // TODO parser api should allow constructing location from a token
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseHtmlRoot(parser, builder);
        const value = builder.make_embedded_value(root, value_symbol_resolve_func, value_replacement_func, value_type_creation_func, value_traversal_func, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the html macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

@no_mangle
public func html_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseHtmlRoot(parser, builder);
        const node = builder.make_embedded_node(root, node_symbol_resolve_func, node_replacement_func, node_known_type_func, node_child_res_func, node_traversal_func, root.parent, loc);
        if(!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the html macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

public func getNextToken(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    if(html.other_mode) {
        if(html.chemical_mode) {
            var nested = lexer.getEmbeddedToken();
            if(nested.type == ChemicalTokenType.LBrace) {
                html.lb_count++;
            } else if(nested.type == ChemicalTokenType.RBrace) {
                html.lb_count--;
                if(html.lb_count == 1) {
                    html.other_mode = false;
                    html.chemical_mode = false;
                }
            }
            return nested;
        }
    }
    if(html.is_comment) {
        const provider = &lexer.provider
        const position = provider.getPosition();
        if(provider.read_comment_text(lexer.str)) {
            // comment has ended
            html.is_comment = false;
        }
        return Token {
            type : TokenType.CommentText as int,
            value : lexer.str.finalize_view(),
            position : position
        }
    }
    const t = getNextToken2(html, lexer);
    return t;
}

@no_mangle
public func html_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(HtmlLexer), alignof(HtmlLexer)) as *mut HtmlLexer;
    new (ptr) HtmlLexer {
        has_lt : false,
        lexed_tag_name : false,
        is_comment : false,
        other_mode : false,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.setUserLexer(ptr, getNextToken)
}