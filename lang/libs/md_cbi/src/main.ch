@no_mangle
public func md_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(MdLexer), alignof(MdLexer)) as *mut MdLexer;
    new (ptr) MdLexer {
        in_fenced_code : false,
        fence_char : '\0',
        fence_count : 0,
        chemical_mode : false,
        lb_count : 0
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}

@no_mangle
public func md_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const loc = intrinsics::get_raw_location();
    // No brace needed, parse until #endmd
    var root = parseMdRoot(parser, builder);
    const type = builder.make_string_type(loc)
    const nodes_arr : []*mut ASTNode = []
    const value = builder.make_embedded_value(std::string_view("md"), root, type, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
    // Consume #endmd token
    if(parser.getToken().type == MdTokenType.EndMd as int) {
        parser.increment();
    }
    return value;
}

@no_mangle
public func md_symResValue(visitor : *mut SymResLinkBody, value : *mut EmbeddedValue) : bool {
    visitor.visitEmbeddedValue(value)
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut MdRoot;
    sym_res_root(root, visitor, loc)
    return true;
}

@no_mangle
public func md_replacementValue(builder : *mut ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut MdRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()

    var converter = MdConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }

    converter.convertMdRoot(root);

    const view = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(&view, loc)
    block_val.setCalculatedValue(strValue)
    return block_val;
}

public func node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

@no_mangle
public func md_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    visitor.visitEmbeddedNode(node)
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut MdRoot;
    sym_res_root(root, visitor, loc)
}

@no_mangle
public func md_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut MdRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = MdConverter {
        builder : builder,
        support : &raw mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    converter.convertMdRoot(root);
    return scope;
}

@no_mangle
public func md_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const loc = intrinsics::get_raw_location();
    // No brace needed, parse until #endmd
    var root = parseMdRoot(parser, builder);
    const nodes_arr : []*mut ASTNode = []
    const node = builder.make_embedded_node(AccessSpecifier.Internal, std::string_view("md"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
    // Consume #endmd token
    if(parser.getToken().type == MdTokenType.EndMd as int) {
        parser.increment();
    }
    return node;
}

public func getNextToken(md : &mut MdLexer, lexer : &mut Lexer) : Token {
    // Handle chemical interpolation mode
    if(md.chemical_mode) {
        var nested = lexer.getEmbeddedToken();
        if(nested.type == ChemicalTokenType.LBrace) {
            md.lb_count++;
        } else if(nested.type == ChemicalTokenType.RBrace) {
            md.lb_count--;
            if(md.lb_count == 0) {
                md.chemical_mode = false;
                return Token { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : nested.position }
            }
        }
        return nested;
    }

    // Delegate the rest of the tokenization to the shared md_parser lexer.
    return getNextToken2(md, lexer);
}
