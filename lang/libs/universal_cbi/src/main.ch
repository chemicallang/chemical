// Universal `#universal` macro lexer entry point.
//
// The body of a `#universal` component is JavaScript/JSX, but it may contain
// `style { … }` blocks that are CSS. This lexer hands the CSS body off to the
// shared CSS lexer (css_parser) for the duration of the block, then restores
// the JS lexer, so the parser can parse a style block with `parseCSSOM`.
public func getNextToken(js : &mut JsLexer, lexer : &mut Lexer) : Token {
    var t = nextJsToken(js, lexer, true)
    if(js.style_pending) {
        js.style_pending = false
        if(t.type == JsTokenType.LBrace as int) {
            // The CSS lexer will consume the matching `}`, so undo the `{`
            // increment nextJsToken just made on the JS brace counter.
            js.lb_count = js.lb_count - 1
            ensure_style_css(js, lexer)
            lexer.setUserLexer(js.style_css, universal_style_css_next as UserLexerSubroutineType)
        }
    }
    if(t.type == JsTokenType.Style as int) {
        js.style_pending = true
    }
    return t
}

func ensure_style_css(js : &mut JsLexer, lexer : &mut Lexer) {
    var css : *mut CSSLexer
    if(js.style_css == null) {
        const alloc = lexer.getFileAllocator()
        const ptr = alloc.allocate_size(sizeof(CSSLexer), alignof(CSSLexer)) as *mut CSSLexer
        new (ptr) CSSLexer {
            other_mode : false,
            chemical_mode : false,
            lb_count : 0,
            at_rule : false,
            start_chemical_lb_count : 1,
            has_chemical_in_value : false,
            tokens_since_colon : 0,
            where_state : CSSLexerWhere.Declaration
        }
        js.style_css = ptr as *mut void
        css = ptr
    } else {
        css = js.style_css as *mut CSSLexer
        css.reset()
    }
    // The style block's opening `{` was consumed by the JS lexer, so seed the
    // CSS lexer as if it had lexed that brace: getNextToken2 then restores the
    // JS user lexer itself when it reads the matching `}` (lb_count == 1).
    css.lb_count = 1
    css.start_chemical_lb_count = 1
}

// The CSS user lexer used while inside a `style { … }` block. Mirrors the
// `#css` wrapper (chemical `${…}` handoff); `getNextToken2` restores the JS
// user lexer when the block's closing `}` is read.
public func universal_style_css_next(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    if(css.other_mode) {
        if(css.chemical_mode) {
            var nested = lexer.getEmbeddedToken()
            if(nested.type == ChemicalTokenType.LBrace) {
                css.lb_count++
            } else if(nested.type == ChemicalTokenType.RBrace) {
                if(css.lb_count == css.start_chemical_lb_count) {
                    css.other_mode = false
                    css.chemical_mode = false
                }
                css.lb_count--
            }
            return nested
        }
    }
    return getNextToken2(css, lexer)
}

// Parses `style { … }` (current token is the `Style` token) into a CSS embedded
// value wrapped as a JS chemical value, exactly like `${ #css { … } }`. The
// class name it denotes is a compile-time constant.
public func universal_parse_style_block(parser : *mut Parser, builder : *mut ASTBuilder, dyn_values : *mut std::vector<*mut Value>) : *mut JsNode {
    const loc = parser.getEncodedLocation(parser.getToken())
    parser.increment() // consume `style`; the next `{` hands off to the CSS lexer
    if(!parser.increment_if(JsTokenType.LBrace as int)) {
        parser.error("expected { after style");
        return null
    }
    var root = parseCSSOM(parser, builder)
    if(!parser.increment_if(TokenType.RBrace as int)) {
        parser.error("expected } to close the style block");
    }
    const type = builder.make_string_type(loc)
    const value = builder.make_embedded_value(std::string_view("css"), root, type, std::span<*mut ASTNode>(null, 0), std::span<*mut Value>(root.dyn_values.data(), root.dyn_values.size()), loc)
    if(dyn_values != null) { dyn_values.push(value) }
    var chem = builder.allocate<JsChemicalValue>()
    new (chem) JsChemicalValue {
        base : JsNode { kind : JsNodeKind.ChemicalValue },
        value : value
    }
    return chem as *mut JsNode
}
