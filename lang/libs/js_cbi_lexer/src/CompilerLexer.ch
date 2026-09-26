/**
 * Shared compiler-side lexer for JavaScript / JSX macro bodies.
 *
 * One implementation with a mode flag, replacing the near-duplicate
 * `getNextToken` in `js_cbi` and `universal_cbi`. `jsx_enabled = false` behaves
 * as plain JavaScript (`<` is comparison/shift, `state` is an identifier, and
 * `??`/`?.`/`**` are not produced); `jsx_enabled = true` runs the JSX state
 * machine. Embedded Chemical `${...}` handling is shared by both modes.
 */
public func nextJsToken(js : &mut JsLexer, lexer : &mut Lexer, jsx_enabled : bool) : Token {
    
    if(js.chemical_mode) {
        var nested = lexer.getEmbeddedToken();
        if(nested.type == ChemicalTokenType.LBrace) {
            js.lb_count++;
        } else if(nested.type == ChemicalTokenType.RBrace) {
            js.lb_count--;
            if(js.lb_count == js.chem_start_lb) {
                js.chemical_mode = false;
            }
        }
        return nested;
    }

    const provider = &mut lexer.provider;
    const position = provider.getPosition();
    const data_ptr = provider.current_data()
    
    // Check if we are in JSX child mode (and not in expression)
    // Child mode = jsx_depth > 0 && !in_jsx_tag && jsx_brace_count == 0
    // But we need to handle transitions.
    
    const is_child = jsx_enabled && js.jsx_depth > 0 && js.in_jsx_tag == 0 && js.jsx_brace_count == 0;
    
    if(is_child) {
        // If we are here, we check if we are at < or { or content. A `${` starts
        // a Chemical embed and must be lexed as a `ChemicalStart` token (the
        // switch below), NOT split into a literal `$` text child plus a `{...}`
        // expression container.
        const p = provider.peek();
        const starts_chem = p == '$' && *(provider.current_data() + 1) == '{';
        if(p != '<' && p != '{' && p != '\0' && !starts_chem) {
             // Read text
             const start_ptr = provider.current_data();
             while(true) {
                 const n = provider.peek();
                 if(n == '<' || n == '{' || n == '\0') {
                     break;
                 }
                 // Stop the text run at an embedded `${...}` so the `${` becomes
                 // its own token instead of being swallowed into the text.
                 if(n == '$' && *(provider.current_data() + 1) == '{') {
                     break;
                 }
                 provider.readCharacter();
             }
             return Token { type : JsTokenType.JSXText as int, value : std::string_view(start_ptr, provider.current_data() - start_ptr), position : position }
        }
    }

    const c = provider.readCharacter();
    
    switch(c) {
        '\0' => {
            return Token { type : JsTokenType.EndOfFile as int, value : std::string_view(""), position : position }
        }
        '$' => {
            if(provider.peek() == '{') {
                provider.readCharacter(); // consume {
                js.chem_start_lb = js.lb_count;
                js.chemical_mode = true;
                js.lb_count++;
                return Token { type : JsTokenType.ChemicalStart as int, value : std::string_view("${"), position : position }
            }
            provider.read_identifier();
            const val = std::string_view(data_ptr, provider.current_data() - data_ptr);
            return Token { type : JsTokenType.Identifier as int, value : val, position : position }
        }
        '{' => {
            js.lb_count++;
            if(jsx_enabled && js.jsx_depth > 0) {
                 js.jsx_brace_count++;
                 // Push: shift up and store current LSB
                 // We limit to 64 levels which is plenty.
                 js.tag_mode_stack = (js.tag_mode_stack << 1) | (js.in_jsx_tag as ubigint);
                 // Inside expression, we are NOT in a tag (unless nested < starts one)
                 js.in_jsx_tag = 0;
            }
            return Token { type : JsTokenType.LBrace as int, value : std::string_view("{"), position : position }
        }
        '}' => {
            if(js.lb_count == 1) {
                js.lb_count = 0;
                js.reset();
                lexer.unsetUserLexer();
            } else if(js.lb_count > 1) {
                js.lb_count--;
            }
            
            if(jsx_enabled && js.jsx_depth > 0 && js.jsx_brace_count > 0) {
                 js.jsx_brace_count--;
                 // Pop: extract LSB and shift down
                 js.in_jsx_tag = (js.tag_mode_stack & 1) as int;
                 js.tag_mode_stack = js.tag_mode_stack >> 1;
            }
            return Token { type : JsTokenType.RBrace as int, value : std::string_view("}"), position : position }
        }
        '(' => { return Token { type : JsTokenType.LParen as int, value : std::string_view("("), position : position } }
        ')' => { return Token { type : JsTokenType.RParen as int, value : std::string_view(")"), position : position } }
        '[' => { return Token { type : JsTokenType.LBracket as int, value : std::string_view("["), position : position } }
        ']' => { return Token { type : JsTokenType.RBracket as int, value : std::string_view("]"), position : position } }
        ';' => { return Token { type : JsTokenType.SemiColon as int, value : std::string_view(";"), position : position } }
        ',' => { return Token { type : JsTokenType.Comma as int, value : std::string_view(","), position : position } }
        ':' => { return Token { type : JsTokenType.Colon as int, value : std::string_view(":"), position : position } }
        '.' => {
            if(provider.peek() == '.') {
                provider.readCharacter();
                if(provider.peek() == '.') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.ThreeDots as int, value : std::string_view("..."), position : position }
                }
                // .. is likely not valid JS, return dot and dot? 
                // But simplified:
            }
            return Token { type : JsTokenType.Dot as int, value : std::string_view("."), position : position }
        }
        '+' => {
            if(provider.peek() == '+') {
                provider.readCharacter();
                return Token { type : JsTokenType.PlusPlus as int, value : std::string_view("++"), position : position }
            } else if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.PlusEqual as int, value : std::string_view("+="), position : position }
            }
            return Token { type : JsTokenType.Plus as int, value : std::string_view("+"), position : position }
        }
        '-' => {
            if(provider.peek() == '-') {
                provider.readCharacter();
                return Token { type : JsTokenType.MinusMinus as int, value : std::string_view("--"), position : position }
            } else if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.MinusEqual as int, value : std::string_view("-="), position : position }
            }
            return Token { type : JsTokenType.Minus as int, value : std::string_view("-"), position : position }
        }
        '*' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.StarEqual as int, value : std::string_view("*="), position : position }
            } else if(jsx_enabled && provider.peek() == '*') {
                provider.readCharacter();
                return Token { type : JsTokenType.Exponent as int, value : std::string_view("**"), position : position }
            }
            return Token { type : JsTokenType.Star as int, value : std::string_view("*"), position : position }
        }
        '%' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.PercentEqual as int, value : std::string_view("%="), position : position }
            }
            return Token { type : JsTokenType.Percent as int, value : std::string_view("%"), position : position }
        }
        '/' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.SlashEqual as int, value : std::string_view("/="), position : position }
            } else if(jsx_enabled && provider.peek() == '>') {
                // /> Self-closing!
                // If we are in_jsx_tag, and we see />, then we should decrement depth because we incremented at <.
                if(js.in_jsx_tag == 1) {
                    if(js.jsx_depth > 0) {
                        js.jsx_depth--;
                        js.jsx_brace_count = (js.jsx_brace_stack & 0xFF) as int;
                        js.jsx_brace_stack >>= 8;
                    }
                    // Note: we don't change in_jsx_tag here, > will do it.
                }
                // Return /
                return Token { type : JsTokenType.Slash as int, value : std::string_view("/"), position : position }
            } else if(provider.peek() == '/') {
                // Single line comment
                provider.readCharacter(); // consume /
                while(true) {
                    const next = provider.peek();
                    if(next == '\n' || next == '\0') {
                        break;
                    }
                    provider.readCharacter();
                }
                // Recursively get next token after comment
                return nextJsToken(js, lexer, jsx_enabled);
            } else if(provider.peek() == '*') {
                // Multi line comment
                provider.readCharacter(); // consume *
                while(true) {
                    const next = provider.peek();
                    if(next == '\0') break;
                    if(next == '*') {
                         provider.readCharacter();
                         if(provider.peek() == '/') {
                             provider.readCharacter();
                             break;
                         }
                         continue;
                    }
                    provider.readCharacter();
                }
                // Recursively get next token after comment
                return nextJsToken(js, lexer, jsx_enabled);
            }
            return Token { type : JsTokenType.Slash as int, value : std::string_view("/"), position : position }
        }
        '&' => {
            if(provider.peek() == '&') {
                provider.readCharacter();
                return Token { type : JsTokenType.LogicalAnd as int, value : std::string_view("&&"), position : position }
            }
            return Token { type : JsTokenType.BitwiseAnd as int, value : std::string_view("&"), position : position }
        }
        '|' => {
            if(provider.peek() == '|') {
                provider.readCharacter();
                return Token { type : JsTokenType.LogicalOr as int, value : std::string_view("||"), position : position }
            }
            return Token { type : JsTokenType.BitwiseOr as int, value : std::string_view("|"), position : position }
        }
        '?' => {
            if(jsx_enabled && provider.peek() == '?') {
                provider.readCharacter();
                return Token { type : JsTokenType.NullishCoalescing as int, value : std::string_view("??"), position : position }
            } else if(jsx_enabled && provider.peek() == '.') {
                provider.readCharacter();
                return Token { type : JsTokenType.QuestionDot as int, value : std::string_view("?."), position : position }
            }
            return Token { type : JsTokenType.Question as int, value : std::string_view("?"), position : position }
        }
        ' ', '\t', '\n', '\r' => {
            provider.skip_whitespaces();
            return nextJsToken(js, lexer, jsx_enabled);
        }
        '=' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                if(provider.peek() == '=') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.StrictEqual as int, value : std::string_view("==="), position : position }
                }
                return Token { type : JsTokenType.EqualEqual as int, value : std::string_view("=="), position : position }
            } else if(provider.peek() == '>') {
                provider.readCharacter();
                return Token { type : JsTokenType.Arrow as int, value : std::string_view("=>"), position : position }
            }
            return Token { type : JsTokenType.Equal as int, value : std::string_view("="), position : position }
        }
        '!' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                if(provider.peek() == '=') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.StrictNotEqual as int, value : std::string_view("!=="), position : position }
                }
                return Token { type : JsTokenType.NotEqual as int, value : std::string_view("!="), position : position }
            }
            return Token { type : JsTokenType.Exclamation as int, value : std::string_view("!"), position : position }
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
                return Token { type : JsTokenType.LessThanEqual as int, value : std::string_view("<="), position : position }
            } else if(provider.peek() == '<') {
                provider.readCharacter();
                return Token { type : JsTokenType.LeftShift as int, value : std::string_view("<<"), position : position }
            }
            
            if(jsx_enabled) {
                // JSX Heuristic
                // <Identifier -> Open Tag
                // </ -> Closing Tag
                // <> -> Fragment Open
            
                const p = provider.peek();
            
                // We need to support reading Identifier to check, but we shouldn't consume it if it's not JSX?
                // Lexer structure returns one token.
                // But we need to update state.
            
                var is_jsx = false;
                var is_closing = false;
            
                if(p == '/') {
                    // </...
                    is_jsx = true;
                    is_closing = true;
                    // We consume /? No, we return <.
                    // But we update state.
                } else if(p == '>') {
                    // <>
                    is_jsx = true;
                } else if(isalpha(p as int) || p == '_' || p == '$' || p == '{') {
                     // <Ident -> likely JSX start?
                     // Or x < y.
                     // In #component macro, or inside JSX, <Ident is treated as JSX tag start.
                     // Outside... standard JS x < y is valid.
                     // Heuristic: If we are already in JSX (depth > 0), then <Ident is definitely nested tag.
                     // If depth == 0, we assume it's JSX start if it looks like one?
                     // Standard JS parser has trouble here too without context.
                     // But for #component, we prefer JSX.
                     is_jsx = true;
                }
            
                if(is_jsx) {
                     if(is_closing) {
                         // </
                         if(js.jsx_depth > 0) {
                             js.jsx_depth--;
                             js.jsx_brace_count = (js.jsx_brace_stack & 0xFF) as int;
                             js.jsx_brace_stack >>= 8;
                         }
                     } else {
                         // <...
                         js.jsx_brace_stack = (js.jsx_brace_stack << 8) | (js.jsx_brace_count as ubigint);
                         js.jsx_brace_count = 0;
                         js.jsx_depth++;
                     }
                     js.in_jsx_tag = 1;
                }
            

            }
            return Token { type : JsTokenType.LessThan as int, value : std::string_view("<"), position : position }
        }
        '>' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token { type : JsTokenType.GreaterThanEqual as int, value : std::string_view(">="), position : position }
            } else if(provider.peek() == '>') {
                provider.readCharacter();
                if(provider.peek() == '>') {
                    provider.readCharacter();
                    return Token { type : JsTokenType.RightShiftUnsigned as int, value : std::string_view(">>>"), position : position }
                }
                return Token { type : JsTokenType.RightShift as int, value : std::string_view(">>"), position : position }
            }
            
            // If we are in_jsx_tag, this > closes the tag.
            // Check for /> self closing.
            // We can't easily look back.
            // But assume parser handles flow.
            // Lexer just needs to know we exited tag.
            
            if(jsx_enabled && js.in_jsx_tag == 1) {
                js.in_jsx_tag = 0;
                
                // If it was self-closing (/>), depth should be decremented?
                // We don't know here easily if we saw /.
                // Actually, if we saw /, we decremented depth at <.
                // Wait. </ ... >.
                // At <, we saw /. We decremented.
                // At >, we just exit tag. Depth is correct.
                
                // What about <div /> ?
                // At <, we saw div. Incremented.
                // At /, we saw >.
                // We need to handle / inside tag?
                // / is handled below.
            }
            
            return Token { type : JsTokenType.GreaterThan as int, value : std::string_view(">"), position : position }
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
            if(isalpha(c as int) || c == '_' || c == '$') {
                provider.read_identifier();
                const val = std::string_view(data_ptr, provider.current_data() - data_ptr);
                const hash = fnv1_hash_view(&val);
                
                switch(hash) {
                    comptime_fnv1_hash("var") => { return Token { type : JsTokenType.Var as int, value : val, position : position } }
                    comptime_fnv1_hash("const") => { return Token { type : JsTokenType.Const as int, value : val, position : position } }
                    comptime_fnv1_hash("let") => { return Token { type : JsTokenType.Let as int, value : val, position : position } }
                    comptime_fnv1_hash("state") => {
                        if(jsx_enabled) { return Token { type : JsTokenType.State as int, value : val, position : position } }
                        return Token { type : JsTokenType.Identifier as int, value : val, position : position }
                    }
                    comptime_fnv1_hash("for") => { return Token { type : JsTokenType.For as int, value : val, position : position } }
                    comptime_fnv1_hash("while") => { return Token { type : JsTokenType.While as int, value : val, position : position } }
                    comptime_fnv1_hash("break") => { return Token { type : JsTokenType.Break as int, value : val, position : position } }
                    comptime_fnv1_hash("continue") => { return Token { type : JsTokenType.Continue as int, value : val, position : position } }
                    comptime_fnv1_hash("switch") => { return Token { type : JsTokenType.Switch as int, value : val, position : position } }
                    comptime_fnv1_hash("case") => { return Token { type : JsTokenType.Case as int, value : val, position : position } }
                    comptime_fnv1_hash("default") => { return Token { type : JsTokenType.Default as int, value : val, position : position } }
                    comptime_fnv1_hash("do") => { return Token { type : JsTokenType.Do as int, value : val, position : position } }
                    comptime_fnv1_hash("try") => { return Token { type : JsTokenType.Try as int, value : val, position : position } }
                    comptime_fnv1_hash("catch") => { return Token { type : JsTokenType.Catch as int, value : val, position : position } }
                    comptime_fnv1_hash("finally") => { return Token { type : JsTokenType.Finally as int, value : val, position : position } }
                    comptime_fnv1_hash("throw") => { return Token { type : JsTokenType.Throw as int, value : val, position : position } }
                    comptime_fnv1_hash("function") => { return Token { type : JsTokenType.Function as int, value : val, position : position } }
                    comptime_fnv1_hash("return") => { return Token { type : JsTokenType.Return as int, value : val, position : position } }
                    comptime_fnv1_hash("if") => { return Token { type : JsTokenType.If as int, value : val, position : position } }
                    comptime_fnv1_hash("else") => { return Token { type : JsTokenType.Else as int, value : val, position : position } }
                    comptime_fnv1_hash("true") => { return Token { type : JsTokenType.True as int, value : val, position : position } }
                    comptime_fnv1_hash("false") => { return Token { type : JsTokenType.False as int, value : val, position : position } }
                    comptime_fnv1_hash("null") => { return Token { type : JsTokenType.Null as int, value : val, position : position } }
                    comptime_fnv1_hash("undefined") => { return Token { type : JsTokenType.Undefined as int, value : std::string_view("undefined"), position : position } }
                    comptime_fnv1_hash("new") => { return Token { type : JsTokenType.New as int, value : val, position : position } }
                    comptime_fnv1_hash("async") => { return Token { type : JsTokenType.Async as int, value : val, position : position } }
                    comptime_fnv1_hash("await") => { return Token { type : JsTokenType.Await as int, value : val, position : position } }
                    comptime_fnv1_hash("this") => { return Token { type : JsTokenType.This as int, value : val, position : position } }
                    comptime_fnv1_hash("of") => { return Token { type : JsTokenType.Of as int, value : val, position : position } }
                    comptime_fnv1_hash("typeof") => { return Token { type : JsTokenType.Typeof as int, value : val, position : position } }
                    comptime_fnv1_hash("void") => { return Token { type : JsTokenType.Void as int, value : val, position : position } }
                    comptime_fnv1_hash("delete") => { return Token { type : JsTokenType.Delete as int, value : val, position : position } }
                    comptime_fnv1_hash("in") => { return Token { type : JsTokenType.In as int, value : val, position : position } }
                    comptime_fnv1_hash("instanceof") => { return Token { type : JsTokenType.InstanceOf as int, value : val, position : position } }
                    comptime_fnv1_hash("class") => { return Token { type : JsTokenType.Class as int, value : val, position : position } }
                    comptime_fnv1_hash("extends") => { return Token { type : JsTokenType.Extends as int, value : val, position : position } }
                    comptime_fnv1_hash("super") => { return Token { type : JsTokenType.Super as int, value : val, position : position } }
                    comptime_fnv1_hash("static") => { return Token { type : JsTokenType.Static as int, value : val, position : position } }
                    comptime_fnv1_hash("import") => { return Token { type : JsTokenType.Import as int, value : val, position : position } }
                    comptime_fnv1_hash("export") => { return Token { type : JsTokenType.Export as int, value : val, position : position } }
                    comptime_fnv1_hash("yield") => { return Token { type : JsTokenType.Yield as int, value : val, position : position } }
                    comptime_fnv1_hash("debugger") => { return Token { type : JsTokenType.Debugger as int, value : val, position : position } }
                    default => {
                        return Token { type : JsTokenType.Identifier as int, value : val, position : position }
                    }
                }
            } else if(isdigit(c)) {
                provider.read_digits();
                return Token { type : JsTokenType.Number as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
            }
            return Token { type : 0, value : std::string_view("unexpected"), position : position }
        }
        '~' => { return Token { type : JsTokenType.BitwiseNot as int, value : std::string_view("~"), position : position } }
        '^' => { return Token { type : JsTokenType.BitwiseXor as int, value : std::string_view("^"), position : position } }
        '#' => { return Token { type : JsTokenType.Hash as int, value : std::string_view("#"), position : position } }
    }
}

