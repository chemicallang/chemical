/**
 * Universal router grammar (`lang/docs/universal-router-design.md` §4, §14.1/§14.2).
 *
 *   router "name" {
 *       route default #"dashboard" { <Dashboard /> }
 *       route #"projects" preload { <Projects /> }
 *       route "/projects/{id}" { <Project /> }
 *       route * { <NotFound /> }
 *   }
 *
 * `router`/`route` are contextual keywords (D-1.2): they stay usable as ordinary
 * identifiers; the dispatcher in `parseStatement` only commits when the token
 * followed by a router-shape token. The `#` sigil is a real token (`Hash`), added
 * in Phase 0 (D-1.1).
 *
 * These declarations live inside an `#universal` component body; `route` is only
 * valid inside a `router` (enforced by the converter, D-1.3), but the parser is
 * permissive so the converter can emit a located diagnostic instead of failing
 * to parse.
 */

func js_peek_token(parser : *mut Parser, ahead : int) : *mut Token {
    var ptr = parser.getTokenPtr()
    return (*ptr) + ahead
}

func js_token_ident_equals(token : *mut Token, value : std::string_view) : bool {
    if(token.type != JsTokenType.Identifier as int) return false
    return token.value.equals(&value)
}

func js_token_ident_equals_cstr(token : *mut Token, value : *char) : bool {
    if(token.type != JsTokenType.Identifier as int) return false
    return token.value.equals(std::string_view(value))
}

// Strips one layer of surrounding quotes from a lexed string token.
func js_strip_quotes(value : std::string_view) : std::string_view {
    if(value.size() < 2) return value
    const first = value.get(0)
    if(first != '"' && first != '\'' && first != '`') return value
    return std::string_view(value.data() + 1, value.size() - 2)
}

// Returns the hook name ("onActivate"/"onDeactivate"/"onBeforeActivate") for an
// identifier, else "".
func js_route_hook_name_from_ident(value : std::string_view) : std::string_view {
    if(value.equals(std::string_view("onActivate"))) return value
    if(value.equals(std::string_view("onDeactivate"))) return value
    if(value.equals(std::string_view("onBeforeActivate"))) return value
    return std::string_view()
}

// Parses `{ ... }`. Hook calls (`onActivate(() => { ... })`) are parsed directly
// and stripped out of the body: parsing them as ordinary expression statements
// would let the expression parser greedily continue across the newline into the
// following JSX root (`hook(...) <Root/>` parses as a comparison). Parsing the
// call and stopping at its closing `)` keeps the JSX root a separate statement.
func js_parse_route_body(parser : *mut Parser, builder : *mut ASTBuilder, jsParser : &mut JsParser,
                         hooks : &mut std::vector<*mut JsNode>) : *mut JsNode {
    if(!parser.increment_if(JsTokenType.LBrace as int)) {
        parser.error("expected { after route declaration")
        return null
    }

    var bodyStatements = std::vector<*mut JsNode>()
    while(parser.getToken().type != JsTokenType.RBrace as int &&
          parser.getToken().type != JsTokenType.EndOfFile as int) {
        const tok = parser.getToken()
        if(tok.type == JsTokenType.Identifier as int &&
           js_peek_token(parser, 1).type == JsTokenType.LParen as int) {
            const hookName = js_route_hook_name_from_ident(tok.value)
            if(hookName.size() > 0) {
                parser.increment() // hook name
                parser.increment() // (
                var fn : *mut JsNode = null
                if(parser.getToken().type != JsTokenType.RParen as int) {
                    fn = jsParser.parseExpression(parser, builder)
                }
                parser.increment_if(JsTokenType.RParen as int)
                parser.increment_if(JsTokenType.SemiColon as int)
                var hook = builder.allocate<JsRouteHook>()
                new (hook) JsRouteHook {
                    base : JsNode { kind : JsNodeKind.RouteHook },
                    name : builder.allocate_view(&hookName),
                    fn : fn
                }
                hooks.push(hook as *mut JsNode)
                continue
            }
        }
        var stmt = jsParser.parseStatement(parser, builder)
        if(stmt == null) break
        bodyStatements.push(stmt)
    }

    if(!parser.increment_if(JsTokenType.RBrace as int)) {
        parser.error("expected } after route declaration")
    }

    var body = builder.allocate<JsBlock>()
    new (body) JsBlock {
        base : JsNode { kind : JsNodeKind.Block },
        statements : bodyStatements
    }
    return body as *mut JsNode
}

func js_parse_route_decl(parser : *mut Parser, builder : *mut ASTBuilder, jsParser : &mut JsParser) : *mut JsNode {
    const declLoc = parser.getEncodedLocation(parser.getToken())
    parser.increment() // consume `route`

    var is_default = parser.increment_if(JsTokenType.Default as int)
    var raw = std::string_view()
    var id = std::string_view()
    var pattern = std::string_view()
    var is_url = false
    var is_fallback = false

    const shape = parser.getToken()
    if(shape.type == JsTokenType.Hash as int) {
        // `route #"id"` — id route (no URL semantics)
        parser.increment()
        const strTok = parser.getToken()
        if(strTok.type != JsTokenType.String as int) {
            parser.error("expected string literal after '#' in route declaration")
            return null
        }
        const stripped = js_strip_quotes(strTok.value)
        var rawBuf = std::string("#")
        rawBuf.append_view(&stripped)
        const rawPtr = builder.allocate_str(rawBuf.data(), rawBuf.size())
        raw = std::string_view(rawPtr, rawBuf.size())
        id = builder.allocate_view(&stripped)
        parser.increment()
    } else if(shape.type == JsTokenType.String as int) {
        // `route "/a/{b}"` — URL route
        const stripped = js_strip_quotes(shape.value)
        raw = builder.allocate_view(&stripped)
        pattern = builder.allocate_view(&stripped)
        id = builder.allocate_view(&stripped)
        is_url = true
        parser.increment()
    } else if(shape.type == JsTokenType.Star as int) {
        // `route *` — fallback. It needs a stable record id so the client can
        // activate it and the server can select it; `"*"` is reserved and cannot
        // collide with an explicit `route #"id"`.
        parser.increment()
        raw = builder.allocate_view(&std::string_view("*"))
        id = builder.allocate_view(&std::string_view("*"))
        is_fallback = true
    } else {
        parser.error("expected '#id', url string, or '*' in route declaration")
        return null
    }

    var mode = std::string_view()
    var noscroll = false
    while(parser.getToken().type == JsTokenType.Identifier as int) {
        const mv = parser.getToken().value
        if(mv.equals(std::string_view("lazy")) || mv.equals(std::string_view("preload")) || mv.equals(std::string_view("remote"))) {
            mode = builder.allocate_view(&mv)
            parser.increment()
        } else if(mv.equals(std::string_view("noscroll"))) {
            noscroll = true
            parser.increment()
        } else {
            break
        }
    }

    var title = std::string_view()
    if(js_token_ident_equals(parser.getToken(), std::string_view("title"))) {
        parser.increment()
        const titleTok = parser.getToken()
        if(titleTok.type != JsTokenType.String as int) {
            parser.error("expected string literal after 'title' in route declaration")
            return null
        }
        const stripped = js_strip_quotes(titleTok.value)
        title = builder.allocate_view(&stripped)
        parser.increment()
    }

    var hooks = std::vector<*mut JsNode>()
    var body = js_parse_route_body(parser, builder, jsParser, &mut hooks)

    var routeDecl = builder.allocate<JsRouteDecl>()
    new (routeDecl) JsRouteDecl {
        base : JsNode { kind : JsNodeKind.RouteDecl },
        raw : raw,
        id : id,
        pattern : pattern,
        is_url : is_url,
        is_fallback : is_fallback,
        is_default : is_default,
        mode : mode,
        noscroll : noscroll,
        title : title,
        hooks : hooks,
        body : body,
        decl_loc : declLoc
    }
    return routeDecl as *mut JsNode
}

func js_parse_router_decl(parser : *mut Parser, builder : *mut ASTBuilder, jsParser : &mut JsParser) : *mut JsNode {
    const declLoc = parser.getEncodedLocation(parser.getToken())
    parser.increment() // consume `router`

    var name = std::string_view()
    const nameTok = parser.getToken()
    if(nameTok.type == JsTokenType.String as int) {
        const stripped = js_strip_quotes(nameTok.value)
        name = builder.allocate_view(&stripped)
        parser.increment()
    } else if(nameTok.type == JsTokenType.Identifier as int) {
        name = builder.allocate_view(&nameTok.value)
        parser.increment()
    }

    if(!parser.increment_if(JsTokenType.LBrace as int)) {
        parser.error("expected { after router declaration")
        return null
    }

    var routes = std::vector<*mut JsNode>()
    while(parser.getToken().type != JsTokenType.RBrace as int &&
          parser.getToken().type != JsTokenType.EndOfFile as int) {
        if(js_token_ident_equals(parser.getToken(), std::string_view("route"))) {
            var route = js_parse_route_decl(parser, builder, jsParser)
            if(route == null) return null
            routes.push(route)
        } else {
            parser.error("expected 'route' declaration inside router block")
            return null
        }
    }

    if(!parser.increment_if(JsTokenType.RBrace as int)) {
        parser.error("expected } after router declaration")
    }

    var routerDecl = builder.allocate<JsRouterDecl>()
    new (routerDecl) JsRouterDecl {
        base : JsNode { kind : JsNodeKind.RouterDecl },
        name : name,
        routes : routes,
        decl_loc : declLoc
    }
    return routerDecl as *mut JsNode
}

/**
 * Contextual-keyword dispatch (D-1.2). Returns null WITHOUT consuming any token
 * when the current statement is not a router/route declaration.
 */
public func tryParseRouterStatement(jsParser : &mut JsParser, parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsNode {
    const token = parser.getToken()
    if(token.type != JsTokenType.Identifier as int) return null

    if(token.value.equals(std::string_view("router"))) {
        const next = js_peek_token(parser, 1)
        if(next.type == JsTokenType.String as int ||
           next.type == JsTokenType.Identifier as int ||
           next.type == JsTokenType.LBrace as int) {
            return js_parse_router_decl(parser, builder, jsParser)
        }
        return null
    }

    if(token.value.equals(std::string_view("route"))) {
        const next = js_peek_token(parser, 1)
        if(next.type == JsTokenType.Hash as int ||
           next.type == JsTokenType.String as int ||
           next.type == JsTokenType.Star as int ||
           next.type == JsTokenType.Default as int) {
            return js_parse_route_decl(parser, builder, jsParser)
        }
        return null
    }

    return null
}
