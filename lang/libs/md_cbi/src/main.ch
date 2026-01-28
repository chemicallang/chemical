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

func isEndMd(provider : *SourceProvider) : bool {
    // Check if current position has "endmd"
    const ptr = provider.data_ptr;
    if(ptr + 5 > provider.data_end) return false;
    if(*ptr != 'e') return false;
    if(*(ptr + 1) != 'n') return false;
    if(*(ptr + 2) != 'd') return false;
    if(*(ptr + 3) != 'm') return false;
    if(*(ptr + 4) != 'd') return false;
    // Make sure it's followed by whitespace or newline or EOF
    if(ptr + 5 >= provider.data_end) return true;
    const d5 = *(ptr + 5);
    return d5 == '\0' || d5 == '\n' || d5 == '\r' || d5 == ' ' || d5 == '\t';
}

func countBackticks(provider : *SourceProvider) : int {
    var count = 0;
    const ptr = provider.data_ptr;
    while(ptr + count < provider.data_end && *(ptr + count) == '`') {
        count++;
    }
    return count;
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
    // No brace needed, parse until #endmd
    var root = parseMdRoot(parser, builder);
    const nodes_arr : []*mut ASTNode = []
    const node = builder.make_embedded_node(std::string_view("md"), root, node_known_type_func, node_child_res_func, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
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

    const provider = &lexer.provider;
    const position = provider.getPosition();
    const data_ptr = provider.current_data();

    // Inside fenced code block - read until closing fence
    if(md.in_fenced_code) {
        // Check for closing fence at start of line
        var backtick_count = countBackticks(provider);
        if(backtick_count >= md.fence_count) {
            // Consume the backticks
            var i = 0;
            while(i < backtick_count) {
                provider.readCharacter();
                i++;
            }
            md.in_fenced_code = false;
            md.fence_count = 0;
            // Skip rest of line
            while(provider.peek() != '\n' && provider.peek() != '\0') {
                provider.readCharacter();
            }
            if(provider.peek() == '\n') {
                provider.readCharacter();
            }
            return Token { type : MdTokenType.FencedCodeEnd as int, value : std::string_view("```"), position : position }
        }
        
        // Read until end of line or closing fence
        while(provider.peek() != '\n' && provider.peek() != '\0') {
            provider.readCharacter();
        }
        const code_line = std::string_view(data_ptr, provider.current_data() - data_ptr);
        if(provider.peek() == '\n') {
            provider.readCharacter();
        }
        return Token { type : MdTokenType.CodeContent as int, value : code_line, position : position }
    }

    const c = provider.readCharacter();
    
    switch(c) {
        '\0' => {
            return Token { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : position }
        }
        '#' => {
            // Check for #endmd
            if(isEndMd(provider)) {
                // Consume "endmd"
                provider.readCharacter(); // e
                provider.readCharacter(); // n
                provider.readCharacter(); // d
                provider.readCharacter(); // m
                provider.readCharacter(); // d
                md.reset();
                lexer.unsetUserLexer();
                return Token { type : MdTokenType.EndMd as int, value : std::string_view("#endmd"), position : position }
            }
            return Token { type : MdTokenType.Hash as int, value : std::string_view("#"), position : position }
        }
        '$' => {
            if(provider.peek() == '{') {
                provider.readCharacter(); // consume {
                md.lb_count = 1;
                md.chemical_mode = true;
                return Token { type : MdTokenType.ChemicalStart as int, value : std::string_view("${"), position : position }
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view("$"), position : position }
        }
        '`' => {
            // Check for fenced code block (```)
            if(provider.peek() == '`') {
                provider.readCharacter(); // second `
                if(provider.peek() == '`') {
                    provider.readCharacter(); // third `
                    // Count any additional backticks
                    var count = 3;
                    while(provider.peek() == '`') {
                        provider.readCharacter();
                        count++;
                    }
                    // Skip spaces before language
                    while(provider.peek() == ' ' || provider.peek() == '\t') {
                        provider.readCharacter();
                    }
                    // Read language identifier
                    const lang_start = provider.current_data();
                    while(provider.peek() != '\n' && provider.peek() != '\r' && provider.peek() != '\0' && provider.peek() != ' ') {
                        provider.readCharacter();
                    }
                    const lang = std::string_view(lang_start, provider.current_data() - lang_start);
                    // Skip rest of line
                    while(provider.peek() != '\n' && provider.peek() != '\0') {
                        provider.readCharacter();
                    }
                    if(provider.peek() == '\n') {
                        provider.readCharacter();
                    }
                    md.in_fenced_code = true;
                    md.fence_char = '`';
                    md.fence_count = count;
                    // Return token with language
                    if(lang.size() > 0) {
                        return Token { type : MdTokenType.FencedCodeStart as int, value : lang, position : position }
                    }
                    return Token { type : MdTokenType.FencedCodeStart as int, value : std::string_view(""), position : position }
                }
                // Just two backticks, treat as inline code
                return Token { type : MdTokenType.Backtick as int, value : std::string_view("``"), position : position }
            }
            return Token { type : MdTokenType.Backtick as int, value : std::string_view("`"), position : position }
        }
        '{' => {
            return Token { type : MdTokenType.LBrace as int, value : std::string_view("{"), position : position }
        }
        '}' => {
            return Token { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : position }
        }
        '*' => { return Token { type : MdTokenType.Star as int, value : std::string_view("*"), position : position } }
        '_' => { return Token { type : MdTokenType.Underscore as int, value : std::string_view("_"), position : position } }
        '[' => { return Token { type : MdTokenType.LBracket as int, value : std::string_view("["), position : position } }
        ']' => { return Token { type : MdTokenType.RBracket as int, value : std::string_view("]"), position : position } }
        '(' => { return Token { type : MdTokenType.LParen as int, value : std::string_view("("), position : position } }
        ')' => { return Token { type : MdTokenType.RParen as int, value : std::string_view(")"), position : position } }
        '!' => { return Token { type : MdTokenType.Exclamation as int, value : std::string_view("!"), position : position } }
        '>' => { return Token { type : MdTokenType.GreaterThan as int, value : std::string_view(">"), position : position } }
        '-' => { return Token { type : MdTokenType.Dash as int, value : std::string_view("-"), position : position } }
        '+' => { return Token { type : MdTokenType.Plus as int, value : std::string_view("+"), position : position } }
        '|' => { return Token { type : MdTokenType.Pipe as int, value : std::string_view("|"), position : position } }
        '~' => { return Token { type : MdTokenType.Tilde as int, value : std::string_view("~"), position : position } }
        ':' => { return Token { type : MdTokenType.Colon as int, value : std::string_view(":"), position : position } }
        '=' => { return Token { type : MdTokenType.Equal as int, value : std::string_view("="), position : position } }
        '^' => { return Token { type : MdTokenType.Caret as int, value : std::string_view("^"), position : position } }
        '.' => { return Token { type : MdTokenType.Dot as int, value : std::string_view("."), position : position } }
        '\n' => { return Token { type : MdTokenType.Newline as int, value : std::string_view("\n"), position : position } }
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' => {
            // Read full number
            while(provider.peek() >= '0' && provider.peek() <= '9') {
                provider.readCharacter();
            }
            return Token { type : MdTokenType.Number as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
        default => {
            while(true) {
                const next = provider.peek();
                if(next == '\0' || next == '#' || next == '*' || next == '_' || next == '[' || next == ']' || 
                   next == '(' || next == ')' || next == '!' || next == '`' || next == '>' || next == '-' || 
                   next == '+' || next == '|' || next == '\n' || next == '{' || next == '}' || next == '$' ||
                   next == '~' || next == ':' || next == '=' || next == '^' || next == '.' ||
                   (next >= '0' && next <= '9')) {
                    break;
                }
                provider.readCharacter();
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
    }
}
