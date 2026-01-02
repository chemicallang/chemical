@no_mangle
public func js_symResNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut JsRoot;
    sym_res_root(root, resolver, loc)
}

@no_mangle
public func js_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var scope = builder.make_scope(root.parent, loc);
    var scope_nodes = scope.getNodes();
    var converter = JsConverter {
        builder : builder,
        support : &mut root.support,
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
public func js_symResValue(resolver : *mut SymbolResolver, value : *EmbeddedValue) : bool {
    const loc = value.getEncodedLocation()
    const root = value.getDataPtr() as *mut JsRoot;
    sym_res_root(root, resolver, loc)
    return true;
}

@no_mangle
public func js_replacementValue(builder : *mut ASTBuilder, value : *EmbeddedValue) : *Value {
    const loc = intrinsics::get_raw_location();
    const root = value.getDataPtr() as *mut JsRoot;
    var block_val = builder.make_block_value(root.parent, loc)
    var scope_nodes = block_val.get_body()
    
    var converter = JsConverter {
        builder : builder,
        support : &mut root.support,
        vec : scope_nodes,
        parent : root.parent,
        str : std::string()
    }
    
    converter.convertJsRoot(root);
    
    const view = builder.allocate_view(converter.str.to_view())
    const strValue = builder.make_string_value(view, loc)
    block_val.setCalculatedValue(strValue)
    return block_val;
}

@no_mangle
public func js_parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const type = builder.make_string_type(loc)
        const value = builder.make_embedded_value(std::string_view("js"), root, type, std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc);
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
    const loc = intrinsics::get_raw_location();
    if(parser.increment_if(JsTokenType.LBrace as int)) {
        var root = parseJsRoot(parser, builder);
        const node = builder.make_embedded_node(std::string_view("js"), root, node_known_type_func, node_child_res_func, std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), root.parent, loc);
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
    
    if(js.chemical_mode) {
        var nested = lexer.getEmbeddedToken();
        if(nested.type == ChemicalTokenType.LBrace) {
            js.lb_count++;
        } else if(nested.type == ChemicalTokenType.RBrace) {
            js.lb_count--;
            // If we drop back to the level where we started chemical mode?
            // Wait, ${ starts chemical mode.
            // { -> lb_count 1 (macro start)
            // ${ -> lb_count 1 (still 1?) No, ${ should probably act like an opening brace for counting?
            // If we treat ${ as {.
            // But getEmbeddedToken returns tokens.
            // If we are in chemical mode, we are parsing chemical code.
            // We need to know when to stop.
            // We stop when we hit the closing brace matching the opening brace of the interpolation.
            // But ${ uses { as the opener.
            // So if we increment lb_count on ${, we should decrement on }.
            
            // Let's say:
            // #js { ... ${ ... } ... }
            // { -> lb=1
            // ${ -> lb=2 (chemical mode on)
            // } -> lb=1 (chemical mode off)
            // } -> lb=0 (macro end)
            
            if(js.lb_count == 1) {
                 // This means we closed the chemical block (assuming we started at 1 and went to 2)
                 // Wait, if we are at lb=1, we are back to JS mode?
                 // If we started chemical mode at lb=1 (transition to 2).
                 // Then when we drop to 1, we are done.
                 js.chemical_mode = false;
                 return Token { type : JsTokenType.RBrace as int, value : view("}"), position : nested.position }
            }
        }
        return nested;
    }

    const provider = &mut lexer.provider;
    const position = provider.getPosition();
    const data_ptr = provider.current_data()
    const c = provider.readCharacter();
    
    switch(c) {
        '\0' => {
            return Token { type : JsTokenType.EndOfFile as int, value : view(""), position : position }
        }
        '$' => {
            if(provider.peek() == '{') {
                provider.readCharacter(); // consume {
                js.lb_count++;
                js.chemical_mode = true;
                return Token { type : JsTokenType.ChemicalStart as int, value : view("${"), position : position }
            }
            // Treat as identifier start or just $
            return Token { type : JsTokenType.Identifier as int, value : view("$"), position : position }
        }
        '{' => {
            js.lb_count++;
            return Token { type : JsTokenType.LBrace as int, value : view("{"), position : position }
        }
        '}' => {
            if(js.lb_count == 1) {
                js.reset();
                lexer.unsetUserLexer();
            } else {
                js.lb_count--;
            }
            return Token { type : JsTokenType.RBrace as int, value : view("}"), position : position }
        }
        '(' => { return Token { type : JsTokenType.LParen as int, value : view("("), position : position } }
        ')' => { return Token { type : JsTokenType.RParen as int, value : view(")"), position : position } }
        '[' => { return Token { type : JsTokenType.LBracket as int, value : view("["), position : position } }
        ']' => { return Token { type : JsTokenType.RBracket as int, value : view("]"), position : position } }
        ';' => { return Token { type : JsTokenType.SemiColon as int, value : view(";"), position : position } }
        ',' => { return Token { type : JsTokenType.Comma as int, value : view(","), position : position } }
        ':' => { return Token { type : JsTokenType.Colon as int, value : view(":"), position : position } }
        '.' => {
            if(provider.peek() == '.') {
                provider.readCharacter();
                if(provider.peek() == '.') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.ThreeDots as int, value : view("..."), position : position }
                }
                // .. is likely not valid JS, return dot and dot? 
                // But simplified:
            }
            return Token { type : JsTokenType.Dot as int, value : view("."), position : position }
        }
        '+' => {
            if(provider.peek() == '+') {
                provider.readCharacter();
                return Token { type : JsTokenType.PlusPlus as int, value : view("++"), position : position }
            } else if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.PlusEqual as int, value : view("+="), position : position }
            }
            return Token { type : JsTokenType.Plus as int, value : view("+"), position : position }
        }
        '-' => {
            if(provider.peek() == '-') {
                provider.readCharacter();
                return Token { type : JsTokenType.MinusMinus as int, value : view("--"), position : position }
            } else if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.MinusEqual as int, value : view("-="), position : position }
            }
            return Token { type : JsTokenType.Minus as int, value : view("-"), position : position }
        }
        '*' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.StarEqual as int, value : view("*="), position : position }
            }
            return Token { type : JsTokenType.Star as int, value : view("*"), position : position }
        }
        '/' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.SlashEqual as int, value : view("/="), position : position }
            }
            // TODO handle comments
            return Token { type : JsTokenType.Slash as int, value : view("/"), position : position }
        }
        '&' => {
            if(provider.peek() == '&') {
                provider.readCharacter();
                return Token { type : JsTokenType.LogicalAnd as int, value : view("&&"), position : position }
            }
            return Token { type : JsTokenType.BitwiseAnd as int, value : view("&"), position : position }
        }
        '|' => {
            if(provider.peek() == '|') {
                provider.readCharacter();
                return Token { type : JsTokenType.LogicalOr as int, value : view("||"), position : position }
            }
            return Token { type : JsTokenType.BitwiseOr as int, value : view("|"), position : position }
        }
        '?' => {
            return Token { type : JsTokenType.Question as int, value : view("?"), position : position }
        }
        ' ', '\t', '\n', '\r' => {
            provider.skip_whitespaces();
            return getNextToken(js, lexer);
        }
        '=' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.EqualEqual as int, value : view("=="), position : position }
            } else if(provider.peek() == '>') {
                provider.readCharacter();
                return Token { type : JsTokenType.Arrow as int, value : view("=>"), position : position }
            }
            return Token { type : JsTokenType.Equal as int, value : view("="), position : position }
        }
        '!' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.NotEqual as int, value : view("!="), position : position }
            }
            return Token { type : JsTokenType.Exclamation as int, value : view("!"), position : position }
        }
        '`' => {
             while(true) {
                 const nc = provider.peek();
                 if(nc == '`') {
                     provider.readCharacter();
                     break;
                 }
                 if(nc == '\0') {
                     break; 
                 }
                 if(nc == '\\') {
                     provider.readCharacter(); 
                     provider.readCharacter(); 
                     continue;
                 }
                 provider.readCharacter();
             }
             return Token { type : JsTokenType.String as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
        '<' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.LessThanEqual as int, value : view("<="), position : position }
            } else if(provider.peek() == '<') {
                provider.readCharacter();
                return Token { type : JsTokenType.LeftShift as int, value : view("<<"), position : position }
            }
            return Token { type : JsTokenType.LessThan as int, value : view("<"), position : position }
        }
        '>' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.GreaterThanEqual as int, value : view(">="), position : position }
            } else if(provider.peek() == '>') {
                provider.readCharacter();
                if(provider.peek() == '>') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.RightShiftUnsigned as int, value : view(">>>"), position : position }
                }
                return Token { type : JsTokenType.RightShift as int, value : view(">>"), position : position }
            }
            return Token { type : JsTokenType.GreaterThan as int, value : view(">"), position : position }
        }
        '"', '\'' => {
            // String literal
            // TODO: handle escaping
            // For now simple string
             // provider.read_string_literal(c);
             // We don't have read_string_literal exposed maybe?
             // html_cbi uses read_double_quoted_value
             if(c == '"') {
                 provider.read_double_quoted_value();
             } else {
                 provider.read_single_quoted_value();
             }
             return Token { type : JsTokenType.String as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
        default => {
            if(isalpha(c as int) || c == '_') {
                provider.read_identifier();
                const val = std::string_view(data_ptr, provider.current_data() - data_ptr);
                if(val.equals(view("var"))) {
                    return Token { type : JsTokenType.Var as int, value : val, position : position }
                } else if(val.equals(view("const"))) {
                    return Token { type : JsTokenType.Const as int, value : val, position : position }
                } else if(val.equals(view("let"))) {
                    return Token { type : JsTokenType.Let as int, value : val, position : position }
                } else if(val.equals(view("for"))) {
                    return Token { type : JsTokenType.For as int, value : val, position : position }
                } else if(val.equals(view("while"))) {
                    return Token { type : JsTokenType.While as int, value : val, position : position }
                } else if(val.equals(view("break"))) {
                    return Token { type : JsTokenType.Break as int, value : val, position : position }
                } else if(val.equals(view("continue"))) {
                    return Token { type : JsTokenType.Continue as int, value : val, position : position }
                } else if(val.equals(view("switch"))) {
                    return Token { type : JsTokenType.Switch as int, value : val, position : position }
                } else if(val.equals(view("case"))) {
                    return Token { type : JsTokenType.Case as int, value : val, position : position }
                } else if(val.equals(view("default"))) {
                    return Token { type : JsTokenType.Default as int, value : val, position : position }
                } else if(val.equals(view("do"))) {
                    return Token { type : JsTokenType.Do as int, value : val, position : position }
                } else if(val.equals(view("try"))) {
                    return Token { type : JsTokenType.Try as int, value : val, position : position }
                } else if(val.equals(view("catch"))) {
                    return Token { type : JsTokenType.Catch as int, value : val, position : position }
                } else if(val.equals(view("finally"))) {
                    return Token { type : JsTokenType.Finally as int, value : val, position : position }
                } else if(val.equals(view("throw"))) {
                    return Token { type : JsTokenType.Throw as int, value : val, position : position }
                } else if(val.equals(view("function"))) {
                    return Token { type : JsTokenType.Function as int, value : val, position : position }
                } else if(val.equals(view("return"))) {
                    return Token { type : JsTokenType.Return as int, value : val, position : position }
                } else if(val.equals(view("if"))) {
                    return Token { type : JsTokenType.If as int, value : val, position : position }
                } else if(val.equals(view("else"))) {
                    return Token { type : JsTokenType.Else as int, value : val, position : position }
                } else if(val.equals(view("true"))) {
                    return Token { type : JsTokenType.True as int, value : val, position : position }
                } else if(val.equals(view("false"))) {
                    return Token { type : JsTokenType.False as int, value : val, position : position }
                } else if(val.equals(view("null"))) {
                    return Token { type : JsTokenType.Null as int, value : val, position : position }
                } else if(val.equals(view("undefined"))) {
                    return Token { type : JsTokenType.Undefined as int, value : view("undefined"), position : position }
                } else if(val.equals(view("new"))) {
                    return Token { type : JsTokenType.New as int, value : val, position : position }
                } else if(val.equals(view("async"))) {
                    return Token { type : JsTokenType.Async as int, value : val, position : position }
                } else if(val.equals(view("await"))) {
                    return Token { type : JsTokenType.Await as int, value : val, position : position }
                } else if(val.equals(view("this"))) {
                    return Token { type : JsTokenType.This as int, value : val, position : position }
                } else if(val.equals(view("of"))) {
                    return Token { type : JsTokenType.Of as int, value : val, position : position }
                } else if(val.equals(view("typeof"))) {
                    return Token { type : JsTokenType.Typeof as int, value : val, position : position }
                } else if(val.equals(view("void"))) {
                    return Token { type : JsTokenType.Void as int, value : val, position : position }
                } else if(val.equals(view("delete"))) {
                    return Token { type : JsTokenType.Delete as int, value : val, position : position }
                } else if(val.equals(view("in"))) {
                    return Token { type : JsTokenType.In as int, value : val, position : position }
                } else if(val.equals(view("instanceof"))) {
                    return Token { type : JsTokenType.InstanceOf as int, value : val, position : position }
                }
                return Token { type : JsTokenType.Identifier as int, value : val, position : position }
            } else if(isdigit(c)) {
                provider.read_digits();
                return Token { type : JsTokenType.Number as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
            }
            return Token { type : 0, value : view("unexpected"), position : position }
        }
        '~' => { return Token { type : JsTokenType.BitwiseNot as int, value : view("~"), position : position } }
        '^' => { return Token { type : JsTokenType.BitwiseXor as int, value : view("^"), position : position } }
    }
}

@no_mangle
public func js_initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(sizeof(JsLexer), alignof(JsLexer)) as *mut JsLexer;
    new (ptr) JsLexer {
        lb_count : 0,
        chemical_mode : false
    }
    lexer.setUserLexer(ptr, getNextToken as UserLexerSubroutineType)
}
