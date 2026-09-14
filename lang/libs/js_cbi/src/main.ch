@no_mangle
public func js_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    visitor.visitEmbeddedNode(node)
    const resolver = visitor.getSymbolResolver();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsRoot;
    sym_res_root(root, visitor, loc)
}

@no_mangle
public func js_replacementNode(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = JsConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    converter.convertJsRoot(root);
    return scope;
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

@no_mangle
public func js_symResValue(visitor : *mut SymResLinkBody, value : *mut EmbeddedValue) : bool {
    visitor.visitEmbeddedValue(value)
    const resolver = visitor.getSymbolResolver();
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut JsRoot;
    sym_res_root(root, visitor, loc)
    return true;
}

@no_mangle
public func js_replacementValue(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    
    var converter = JsConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    
    converter.convertJsRoot(root);
    
    const view2 = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(&view2, loc)
    block_val.setCalculatedValue(strValue)
    return block_val;
}

@no_mangle
public func js_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const type = builder.make_string_type(loc)
        const nodes_arr : []*mut ASTNode = []
        const value = builder.make_embedded_value(std::string_view("js"), root, type, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the js macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

@no_mangle
public func js_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("js"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(JsTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the js macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

public func getNextToken(js : &mut JsLexer, lexer : &mut Lexer) : Token {
    return nextJsToken(js, lexer, false)
}
@no_mangle
public func js_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer { }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
