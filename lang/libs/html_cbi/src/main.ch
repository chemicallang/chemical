@no_mangle
public func html_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut HtmlRoot;
    sym_res_root(root, resolver, loc)
}

@no_mangle
public func html_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut HtmlRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = ASTConverter {
        builder : builder,
        support : &mut root.support,
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

@no_mangle
public func html_traversalNode(node : *EmbeddedNode, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = node.getDataPtr() as *mut HtmlRoot;
    traverse_root(root, data, traverse)
}

@no_mangle
public func html_symResValue(resolver : *mut SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut HtmlRoot;
    sym_res_root(root, resolver, loc)
}

@no_mangle
public func html_replacementValue(builder : *mut ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut HtmlRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    var converter = ASTConverter {
        builder : builder,
        support : &mut root.support,
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

@no_mangle
public func html_traversalValue(value : *EmbeddedValue, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    const root = value.getDataPtr() as *mut HtmlRoot;
    traverse_root(root, data, traverse)
}

@no_mangle
public func html_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    // TODO parser api should allow constructing location from a token
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(TokenType.LBrace as int)) {
        var root = parseHtmlRoot(parser, builder);
        const type = builder.make_string_type(loc)
        const value = builder.make_embedded_value(std::string_view("html"), root, type, loc);
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
        const node = builder.make_embedded_node(std::string_view("html"), root, node_known_type_func, node_child_res_func, root.parent, loc);
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
        const data_ptr = provider.current_data()
        const has_end = provider.read_comment_text()
        if(has_end) {
            // comment has ended
            html.is_comment = false;
        }
        var end_offset = 0
        if(has_end) {
            end_offset = 3
        }
        return Token {
            type : TokenType.CommentText as int,
            value : std::string_view(data_ptr, (provider.current_data() - end_offset) - data_ptr),
            position : position
        }
    }
    const t = getNextToken2(html, lexer);
    // printf("created token : %.*s with type %d\n", t.value.size(), t.value.data(), t.type);
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
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}