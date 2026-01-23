@no_mangle
public func md_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(MdLexer), alignof(MdLexer)) as *mut MdLexer;
    new (ptr) MdLexer {
        lb_count : 0,
        chemical_mode : false
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}

@no_mangle
public func md_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const loc = intrinsics::get_raw_location();
    // Skip whitespace before {
    while(parser.getToken().type == MdTokenType.Text as int) {
        const txt = parser.getToken().value;
        var all_ws = true;
        var i = 0u;
        while(i < txt.size()) {
            const c = txt.data()[i];
            if(c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                all_ws = false;
                break;
            }
            i++;
        }
        if(all_ws) {
            parser.increment();
        } else {
            break;
        }
    }
    if(parser.increment_if(MdTokenType.LBrace as int)) {
        var root = parseMdRoot(parser, builder);
        const type = builder.make_string_type(loc)
        const nodes_arr : []*mut ASTNode = []
        const value = builder.make_embedded_value(std::string_view("md"), root, type, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
        if(!parser.increment_if(MdTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the md macro");
        }
        return value;
    } else {
        parser.error("expected a lbrace");
    }
    return null;
}

@no_mangle
public func md_symResValue(resolver : *mut SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut MdRoot;
    sym_res_root(root, resolver, loc)
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
        support : &mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    
    converter.convertMdRoot(root);
    
    const view = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(view, loc)
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
public func md_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut MdRoot;
    sym_res_root(root, resolver, loc)
}

@no_mangle
public func md_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut MdRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = MdConverter {
        builder : builder,
        support : &mut root.support,
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
    // Skip whitespace before {
    while(parser.getToken().type == MdTokenType.Text as int) {
        const txt = parser.getToken().value;
        var all_ws = true;
        var i = 0u;
        while(i < txt.size()) {
            const c = txt.data()[i];
            if(c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                all_ws = false;
                break;
            }
            i++;
        }
        if(all_ws) {
            parser.increment();
        } else {
            break;
        }
    }
    if(parser.increment_if(MdTokenType.LBrace as int)) {
        var root = parseMdRoot(parser, builder);
        const nodes_arr : []*mut ASTNode = []
        const node = builder.make_embedded_node(std::string_view("md"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
        if(!parser.increment_if(MdTokenType.RBrace as int)) {
            parser.error("expected a rbrace for ending the md macro");
        }
        return node;
    } else {
        parser.error("expected a lbrace");
        return null;
    }
}

public func getNextToken(md : &mut MdLexer, lexer : &mut Lexer) : Token {
    if(md.chemical_mode) {
        var nested = lexer.getEmbeddedToken();
        if(nested.type == ChemicalTokenType.LBrace) {
            md.lb_count++;
        } else if(nested.type == ChemicalTokenType.RBrace) {
            md.lb_count--;
            if(md.lb_count == 1) {
                 md.chemical_mode = false;
                 return Token { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : nested.position }
            }
        }
        return nested;
    }

    const provider = &lexer.provider;
    const position = provider.getPosition();
    const data_ptr = provider.current_data()

    const c = provider.readCharacter();
    
    switch(c) {
        '\0' => {
            return Token { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : position }
        }
        '$' => {
            if(provider.peek() == '{') {
                provider.readCharacter(); // consume {
                md.lb_count++;
                md.chemical_mode = true;
                return Token { type : MdTokenType.ChemicalStart as int, value : std::string_view("${"), position : position }
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view("$"), position : position }
        }
        '{' => {
            md.lb_count++;
            return Token { type : MdTokenType.LBrace as int, value : std::string_view("{"), position : position }
        }
        '}' => {
            if(md.lb_count == 1) {
                md.reset();
                lexer.unsetUserLexer();
            } else {
                md.lb_count--;
            }
            return Token { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : position }
        }
        '#' => { return Token { type : MdTokenType.Hash as int, value : std::string_view("#"), position : position } }
        '*' => { return Token { type : MdTokenType.Star as int, value : std::string_view("*"), position : position } }
        '_' => { return Token { type : MdTokenType.Underscore as int, value : std::string_view("_"), position : position } }
        '[' => { return Token { type : MdTokenType.LBracket as int, value : std::string_view("["), position : position } }
        ']' => { return Token { type : MdTokenType.RBracket as int, value : std::string_view("]"), position : position } }
        '(' => { return Token { type : MdTokenType.LParen as int, value : std::string_view("("), position : position } }
        ')' => { return Token { type : MdTokenType.RParen as int, value : std::string_view(")"), position : position } }
        '!' => { return Token { type : MdTokenType.Exclamation as int, value : std::string_view("!"), position : position } }
        '`' => { return Token { type : MdTokenType.Backtick as int, value : std::string_view("`"), position : position } }
        '>' => { return Token { type : MdTokenType.GreaterThan as int, value : std::string_view(">"), position : position } }
        '-' => { return Token { type : MdTokenType.Dash as int, value : std::string_view("-"), position : position } }
        '+' => { return Token { type : MdTokenType.Plus as int, value : std::string_view("+"), position : position } }
        '|' => { return Token { type : MdTokenType.Pipe as int, value : std::string_view("|"), position : position } }
        '\n' => { return Token { type : MdTokenType.Newline as int, value : std::string_view("\n"), position : position } }
        default => {
            while(true) {
                const next = provider.peek();
                if(next == '\0' || next == '#' || next == '*' || next == '_' || next == '[' || next == ']' || 
                   next == '(' || next == ')' || next == '!' || next == '`' || next == '>' || next == '-' || 
                   next == '+' || next == '|' || next == '\n' || next == '{' || next == '}' || next == '$') {
                    break;
                }
                provider.readCharacter();
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
    }
}
