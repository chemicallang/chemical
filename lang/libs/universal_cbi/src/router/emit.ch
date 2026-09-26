/**
 * Universal router conversion (lang/docs/universal-router-design.md §4.3, §15.3).
 *
 * A `router "name" { route #"id" { <Comp/> } ... }` declaration inside a
 * `#universal` component body is turned into:
 *
 *   SSR (pageHtml):
 *     <div class="chx-route" id="rN" data-uni-route="name#id"
 *          data-uni-route-active="true|false" tabindex="-1">
 *       <span data-chx-i id="uN"> ...route body SSR... </span>
 *     </div>
 *
 *   client (pageJs): the runtime (once), the registry object (once), and one
 *     `$__uni_route_register(...)` stub per route.
 *
 *   pageJsEnd: the initial `$__uni_activate_initial(...)` tail.
 *
 * Route bodies deliberately do NOT go through `$__uni_hydration_queue`
 * (INV-7): they are registered and mounted only by the router (design §2.2.5).
 *
 * Phase 3 scope: id routes (`route #"id"`) and `route *`. URL patterns are
 * treated as opaque ids until the URL layer lands (Phase 5).
 */

// True when a component body declares at least one router. Used by
// `universal_replacementNode` to enter the emission path for router-only bodies
// that have no top-level `return` (the design's canonical `App` shape).
func router_has_decl(block : *mut JsBlock) : bool {
    if(block == null) return false
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i)
        if(stmt != null && stmt.kind == JsNodeKind.RouterDecl) return true
    }
    return false
}

// The declared default route id, or the first route's id when none is declared.
func router_default_id(rd : *mut JsRouterDecl) : std::string_view {
    var first = std::string_view()
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.is_fallback) continue
        if(r.is_default) return r.id
        if(first.size() == 0) first = r.id
    }
    return first
}

// The single JSX root of a route body, or null.
func router_route_root(route : *mut JsRouteDecl) : *mut JsNode {
    if(route.body == null || route.body.kind != JsNodeKind.Block) return null
    const block = route.body as *mut JsBlock
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i)
        if(stmt == null) continue
        if(stmt.kind == JsNodeKind.ExpressionStatement) {
            const es = stmt as *mut JsExpressionStatement
            const expr = unwrap_returned_jsx_node(es.expression)
            if(expr != null && is_jsx_node(expr)) return expr
        } else if(stmt.kind == JsNodeKind.Return) {
            const ret = stmt as *mut JsReturn
            const val = unwrap_returned_jsx_node(ret.value)
            if(val != null && is_jsx_node(val)) return val
        }
    }
    return null
}

// Returns the arrow-function node of a route hook, or null.
func router_find_hook(route : *mut JsRouteDecl, hookName : std::string_view) : *mut JsNode {
    for(var i : uint = 0; i < route.hooks.size(); i++) {
        const hn = route.hooks.get(i)
        if(hn == null || hn.kind != JsNodeKind.RouteHook) continue
        const h = hn as *mut JsRouteHook
        if(h.name.equals(&hookName)) return h.fn
    }
    return null
}

// The client function name of a route body whose root is a universal component,
// or an empty string for a static route (`comp: null`, §2.4).
func router_route_comp(root : *mut JsNode) : std::string {
    if(root == null || root.kind != JsNodeKind.JSXElement) return std::string()
    const el = root as *mut JsJSXElement
    const sig = el.componentSignature
    if(sig == null) return std::string()
    var name = std::string()
    if(sig.hydrateFunctionNode != null) {
        get_module_scoped_name(sig.hydrateFunctionNode, sig.hydrateName, &mut name)
    } else {
        get_module_scoped_name(sig.functionNode, sig.name, &mut name)
    }
    return name
}

func router_html_escape(v : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < v.size(); i++) {
        const c = v.get(i)
        if(c == '"') { out.append_view(std::string_view("&quot;")) }
        else if(c == '&') { out.append_view(std::string_view("&amp;")) }
        else if(c == '<') { out.append_view(std::string_view("&lt;")) }
        else if(c == '>') { out.append_view(std::string_view("&gt;")) }
        else { out.append(c) }
    }
}

func router_js_escape(v : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < v.size(); i++) {
        const c = v.get(i)
        if(c == '\\') { out.append_view(std::string_view("\\\\")) }
        else if(c == '"') { out.append_view(std::string_view("\\\"")) }
        else if(c == '\n') { out.append_view(std::string_view("\\n")) }
        else if(c == '\r') { out.append_view(std::string_view("\\r")) }
        else { out.append(c) }
    }
}

// Scores a declared pattern for precedence (D-6.2): literal segments and
// `{param}` segments.
func router_pattern_score(pattern : std::string_view, literalCount : &mut int, paramCount : &mut int) {
    var i : size_t = 0
    while(i < pattern.size()) {
        while(i < pattern.size() && pattern.get(i) == '/') { i = i + 1 }
        if(i >= pattern.size()) { break }
        const start = i
        while(i < pattern.size() && pattern.get(i) != '/') { i = i + 1 }
        const seg = pattern.subview(start, i)
        if(seg.size() >= 2 && seg.get(0) == '{') { *paramCount = *paramCount + 1 }
        else { *literalCount = *literalCount + 1 }
    }
}

// True when URL route `a` must be tried before URL route `b`:
// more literal segments → fewer params → declaration order (D-6.2).
func router_route_precedes(rd : *mut JsRouterDecl, a : size_t, b : size_t) : bool {
    var la = 0
    var pa = 0
    router_pattern_score((rd.routes.get(a) as *mut JsRouteDecl).pattern, &mut la, &mut pa)
    var lb = 0
    var pb = 0
    router_pattern_score((rd.routes.get(b) as *mut JsRouteDecl).pattern, &mut lb, &mut pb)
    if(la != lb) { return la > lb }
    if(pa != pb) { return pa < pb }
    return a < b
}

// The URL-route indices in emission (precedence) order, with the fallback last
// (D-6.9). Emitting the table/spec already sorted means both the server matcher
// and the client matcher are a straight first-match scan.
func router_url_order(rd : *mut JsRouterDecl) : std::vector<size_t> {
    var order = std::vector<size_t>()
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        if((rn as *mut JsRouteDecl).is_url) { order.push(i as size_t) }
    }
    // insertion sort (stable via the final `a < b` tie-break)
    for(var a : size_t = 1; a < order.size(); a++) {
        const key = order.get(a)
        var b = a
        while(b > 0 && router_route_precedes(rd, key, order.get(b - 1))) {
            *order.get_ref(b) = order.get(b - 1)
            b = b - 1
        }
        *order.get_ref(b) = key
    }
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        if((rn as *mut JsRouteDecl).is_fallback) { order.push(i as size_t) }
    }
    return order
}

// Builds the compact server match spec consumed by `apply_route_url`:
// one `<id>\t<pattern>\t<is_fallback>\n` line per URL route plus the fallback,
// in precedence order. Id routes are omitted (they have no URL semantics).
func router_match_spec(rd : *mut JsRouterDecl, out : &mut std::string) {
    const order = router_url_order(rd)
    for(var k : size_t = 0; k < order.size(); k++) {
        var r = rd.routes.get(order.get(k)) as *mut JsRouteDecl
        out.append_view(&r.id)
        out.append('\t')
        out.append_view(&r.pattern)
        out.append('\t')
        if(r.is_fallback) { out.append('1') } else { out.append('0') }
        out.append('\n')
    }
}

// Splits a declared pattern into `{param}` names.
func router_pattern_param_names(pattern : std::string_view) : std::vector<std::string_view> {
    var out = std::vector<std::string_view>()
    var i : size_t = 0
    while(i < pattern.size()) {
        while(i < pattern.size() && pattern.get(i) == '/') { i = i + 1 }
        if(i >= pattern.size()) { break }
        const start = i
        while(i < pattern.size() && pattern.get(i) != '/') { i = i + 1 }
        const seg = pattern.subview(start, i)
        if(seg.size() >= 2 && seg.get(0) == '{' && seg.get(seg.size() - 1) == '}') {
            out.push(seg.subview(1, seg.size() - 1))
        }
    }
    return out
}

// Injects one synthetic SSR attribute per `{param}` into a route body's JSX root
// (D-2.7). SSR-only: the route body root is never client-emitted.
func (converter : &mut JsConverter) router_inject_param_props(route : *mut JsRouteDecl, root : *mut JsNode) {
    if(!route.is_url || root == null || root.kind != JsNodeKind.JSXElement) return
    const names = router_pattern_param_names(route.pattern)
    if(names.size() == 0) return

    const el = root as *mut JsJSXElement
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const getParamFn = converter.support.pageNode.child("get_parameter_text")

    for(var i : uint = 0; i < names.size(); i++) {
        const name = names.get(i)
        var already = false
        for(var a : uint = 0; a < el.opening.attributes.size(); a++) {
            const existing = el.opening.attributes.get(a)
            if(existing != null && existing.kind == JsNodeKind.JSXAttribute) {
                if((existing as *mut JsJSXAttribute).name.equals(&name)) { already = true }
            }
        }
        if(already) continue

        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var fnId = builder.make_identifier(std::string_view("get_parameter_text"), getParamFn, false, location)
        const chain = builder.make_access_chain(&std::span<*mut Value>([ pageId, fnId ]), location)
        var call = builder.make_function_call_value(chain, location)
        call.get_args().push(converter.router_string_value(name))

        var chem = builder.allocate<JsChemicalValue>()
        new (chem) JsChemicalValue {
            base : JsNode { kind : JsNodeKind.ChemicalValue },
            value : call as *mut Value
        }
        var container = builder.allocate<JsJSXExpressionContainer>()
        new (container) JsJSXExpressionContainer {
            base : JsNode { kind : JsNodeKind.JSXExpressionContainer },
            expression : chem as *mut JsNode
        }
        var attr = builder.allocate<JsJSXAttribute>()
        new (attr) JsJSXAttribute {
            base : JsNode { kind : JsNodeKind.JSXAttribute },
            name : builder.allocate_view(&name),
            value : container as *mut JsNode,
            loc : location
        }
        el.opening.attributes.push(attr as *mut JsNode)
    }
}

// Emits a declared pattern (`/projects/{id}`) as a JS array of segment strings:
// `"projects", "{id}"` (used inside the client match table's `pattern: [...]`).
func router_pattern_segments_js(pattern : std::string_view, out : &mut std::string) {
    var first = true
    var i : size_t = 0
    while(i < pattern.size()) {
        while(i < pattern.size() && pattern.get(i) == '/') { i = i + 1 }
        if(i >= pattern.size()) { break }
        const start = i
        while(i < pattern.size() && pattern.get(i) != '/') { i = i + 1 }
        if(!first) out.append_view(", ")
        first = false
        out.append('"')
        router_js_escape(pattern.subview(start, i), out)
        out.append('"')
    }
}

// Emits `{param}` names as `"name": null` placeholders for a route's `baseProps`
// (D-2.7): the client matcher overwrites them with the resolved values, but the
// key must exist so hydration is never handed a missing prop.
func router_pattern_params_js(pattern : std::string_view, out : &mut std::string) {
    var first = true
    var i : size_t = 0
    while(i < pattern.size()) {
        while(i < pattern.size() && pattern.get(i) == '/') { i = i + 1 }
        if(i >= pattern.size()) { break }
        const start = i
        while(i < pattern.size() && pattern.get(i) != '/') { i = i + 1 }
        const seg = pattern.subview(start, i)
        if(seg.size() >= 2 && seg.get(0) == '{' && seg.get(seg.size() - 1) == '}') {
            if(!first) out.append_view(", ")
            first = false
            out.append('"')
            router_js_escape(seg.subview(1, seg.size() - 1), out)
            out.append_view("\": null")
        }
    }
}

// ── builder helpers ────────────────────────────────────────────────────────

func (converter : &mut JsConverter) router_page_stmt(method : std::string_view) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const fnNode = converter.support.pageNode.child(&method)
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
    var id = builder.make_identifier(&method, fnNode, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    return builder.make_function_call_node(chain, converter.parent, location)
}

func (converter : &mut JsConverter) router_page_value(method : std::string_view) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const fnNode = converter.support.pageNode.child(&method)
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
    var id = builder.make_identifier(&method, fnNode, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    return builder.make_function_call_value(chain, location)
}

func (converter : &mut JsConverter) router_string_value(text : std::string_view) : *mut Value {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const v = builder.allocate_view(&text)
    return builder.make_string_value(&v, location) as *mut Value
}

// Flushes any pending target text, then appends raw JS text to pageJs. The
// target is restored so the surrounding SSR emission continues normally.
func (converter : &mut JsConverter) router_emit_js(text : &std::string) {
    if(text.size() == 0) return
    converter.put_chain_in()
    const prev = converter.target
    converter.target = BufferType.JavaScript
    converter.str.append_view(text.to_view())
    converter.put_chain_in()
    converter.target = prev
}

// Appends `text` to the current target buffer (HTML during the SSR pass).
func (converter : &mut JsConverter) router_emit_target(text : &std::string) {
    converter.str.append_view(text.to_view())
    converter.put_chain_in()
}

// ── diagnostics (design §14.8) ─────────────────────────────────────────────
//
// All router diagnostics are converter diagnostics with a source location; the
// compiler never crashes on bad router code (G-3). Messages must match the
// frozen catalogue verbatim (G-4).

func (converter : &mut JsConverter) router_diag(msg : &std::string, loc : ubigint) {
    if(converter.diagnoser == null) return
    converter.diagnoser.error(&msg.to_view(), loc)
}

// Counts the JSX roots in a route body (used to detect zero/multiple roots).
func router_count_jsx_roots(route : *mut JsRouteDecl) : int {
    if(route.body == null || route.body.kind != JsNodeKind.Block) return 0
    const block = route.body as *mut JsBlock
    var count = 0
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i)
        if(stmt == null) continue
        if(stmt.kind == JsNodeKind.ExpressionStatement) {
            const es = stmt as *mut JsExpressionStatement
            const expr = unwrap_returned_jsx_node(es.expression)
            if(expr != null && is_jsx_node(expr)) count = count + 1
        } else if(stmt.kind == JsNodeKind.Return) {
            const ret = stmt as *mut JsReturn
            const val = unwrap_returned_jsx_node(ret.value)
            if(val != null && is_jsx_node(val)) count = count + 1
        }
    }
    return count
}

// R1: a `route` declaration outside any router block.
func (converter : &mut JsConverter) router_diag_orphan_route(route : *mut JsRouteDecl) {
    var msg = std::string("'route' declaration is only valid inside a router block")
    converter.router_diag(&msg, route.decl_loc)
}

// True when a node (or descendant) references a `$__uni_*` runtime internal
// (R14). Walks the common expression/statement/JSX node kinds; unknown kinds are
// treated as leaf and cannot contain a reference.
func router_scan_internals(node : *mut JsNode) : bool {
    if(node == null) return false
    switch(node.kind) {
        JsNodeKind.Identifier => {
            return (node as *mut JsIdentifier).value.starts_with(&std::string_view("$__uni_"))
        }
        JsNodeKind.MemberAccess => {
            const m = node as *mut JsMemberAccess
            if(m.property.starts_with(&std::string_view("$__uni_"))) return true
            return router_scan_internals(m.object)
        }
        JsNodeKind.FunctionCall => {
            const c = node as *mut JsFunctionCall
            if(router_scan_internals(c.callee)) return true
            for(var i : uint = 0; i < c.args.size(); i++) {
                if(router_scan_internals(c.args.get(i))) return true
            }
            return false
        }
        JsNodeKind.ExpressionStatement => {
            return router_scan_internals((node as *mut JsExpressionStatement).expression)
        }
        JsNodeKind.Block => {
            const b = node as *mut JsBlock
            for(var i : uint = 0; i < b.statements.size(); i++) {
                if(router_scan_internals(b.statements.get(i))) return true
            }
            return false
        }
        JsNodeKind.If => {
            const s = node as *mut JsIf
            if(router_scan_internals(s.condition)) return true
            if(router_scan_internals(s.thenBlock)) return true
            return router_scan_internals(s.elseBlock)
        }
        JsNodeKind.Return => { return router_scan_internals((node as *mut JsReturn).value) }
        JsNodeKind.VarDecl => { return router_scan_internals((node as *mut JsVarDecl).value) }
        JsNodeKind.ArrowFunction => { return router_scan_internals((node as *mut JsArrowFunction).body) }
        JsNodeKind.BinaryOp => {
            const s = node as *mut JsBinaryOp
            return router_scan_internals(s.left) || router_scan_internals(s.right)
        }
        JsNodeKind.Ternary => {
            const s = node as *mut JsTernary
            return router_scan_internals(s.condition) || router_scan_internals(s.consequent) || router_scan_internals(s.alternate)
        }
        JsNodeKind.UnaryOp => { return router_scan_internals((node as *mut JsUnaryOp).operand) }
        JsNodeKind.Paren => { return router_scan_internals((node as *mut JsParen).expression) }
        JsNodeKind.Spread => { return router_scan_internals((node as *mut JsSpread).argument) }
        JsNodeKind.ArrayLiteral => {
            const a = node as *mut JsArrayLiteral
            for(var i : uint = 0; i < a.elements.size(); i++) {
                if(router_scan_internals(a.elements.get(i))) return true
            }
            return false
        }
        JsNodeKind.ObjectLiteral => {
            const o = node as *mut JsObjectLiteral
            for(var i : uint = 0; i < o.properties.size(); i++) {
                if(router_scan_internals(o.properties.get(i).value)) return true
            }
            return false
        }
        JsNodeKind.IndexAccess => {
            const s = node as *mut JsIndexAccess
            return router_scan_internals(s.object) || router_scan_internals(s.index)
        }
        JsNodeKind.JSXElement => {
            const el = node as *mut JsJSXElement
            for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                if(router_scan_internals(el.opening.attributes.get(i))) return true
            }
            for(var i : uint = 0; i < el.children.size(); i++) {
                if(router_scan_internals(el.children.get(i))) return true
            }
            return false
        }
        JsNodeKind.JSXExpressionContainer => {
            return router_scan_internals((node as *mut JsJSXExpressionContainer).expression)
        }
        JsNodeKind.JSXAttribute => {
            return router_scan_internals((node as *mut JsJSXAttribute).value)
        }
        JsNodeKind.JSXSpreadAttribute => {
            return router_scan_internals((node as *mut JsJSXSpreadAttribute).argument)
        }
        JsNodeKind.JSXFragment => {
            const f = node as *mut JsJSXFragment
            for(var i : uint = 0; i < f.children.size(); i++) {
                if(router_scan_internals(f.children.get(i))) return true
            }
            return false
        }
        default => { return false }
    }
}

// R8: `router("name").<verb>("literal-id")` calls whose id is not declared.
// `routerName`/`validIds` are the router currently being validated (same
// component scope as the call in the common inline case).
func (converter : &mut JsConverter) router_validate_calls(node : *mut JsNode, routerName : std::string_view,
                                                         validIds : &std::vector<*mut JsNode>) {
    if(node == null || converter.diagnoser == null) return
    switch(node.kind) {
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall
            if(call.callee != null && call.callee.kind == JsNodeKind.MemberAccess) {
                const mem = call.callee as *mut JsMemberAccess
                const prop = mem.property
                if(prop.equals(std::string_view("activateRoute")) || prop.equals(std::string_view("preload")) ||
                   prop.equals(std::string_view("buildPath")) || prop.equals(std::string_view("replaceRoute"))) {
                    // `router("name")` receiver
                    if(mem.object != null && mem.object.kind == JsNodeKind.FunctionCall) {
                        const recv = mem.object as *mut JsFunctionCall
                        if(recv.callee != null && recv.callee.kind == JsNodeKind.Identifier &&
                           (recv.callee as *mut JsIdentifier).value.equals(std::string_view("router")) &&
                           recv.args.size() > 0 && recv.args.get(0).kind == JsNodeKind.Literal &&
                           call.args.size() > 0 && call.args.get(0).kind == JsNodeKind.Literal) {
                            const nameLit = (recv.args.get(0) as *mut JsLiteral).value
                            const idLit = (call.args.get(0) as *mut JsLiteral).value
                            // Only validate the router this component declares.
                            if(nameLit.contains(&routerName) || routerName.contains(&nameLit)) {
                                var found = false
                                for(var i : uint = 0; i < validIds.size(); i++) {
                                    var r = validIds.get(i) as *mut JsRouteDecl
                                    var quoted = std::string("\"")
                                    quoted.append_view(&r.id)
                                    quoted.append('"')
                                    if(idLit.equals(quoted.to_view())) { found = true }
                                }
                                if(!found) {
                                    var msg = std::string("no route '")
                                    msg.append_view(&idLit)
                                    msg.append_view("' in router \"")
                                    msg.append_view(&routerName)
                                    msg.append_view("\"")
                                    converter.router_diag(&msg, call.loc)
                                }
                            }
                        }
                    }
                }
            }
            if(router_scan_internals(call.callee)) { }
            for(var i : uint = 0; i < call.args.size(); i++) {
                converter.router_validate_calls(call.args.get(i), routerName, validIds)
            }
            converter.router_validate_calls(call.callee, routerName, validIds)
        }
        JsNodeKind.ExpressionStatement => { converter.router_validate_calls((node as *mut JsExpressionStatement).expression, routerName, validIds) }
        JsNodeKind.Return => { converter.router_validate_calls((node as *mut JsReturn).value, routerName, validIds) }
        JsNodeKind.VarDecl => { converter.router_validate_calls((node as *mut JsVarDecl).value, routerName, validIds) }
        JsNodeKind.ArrowFunction => { converter.router_validate_calls((node as *mut JsArrowFunction).body, routerName, validIds) }
        JsNodeKind.Block => {
            const b = node as *mut JsBlock
            for(var i : uint = 0; i < b.statements.size(); i++) {
                converter.router_validate_calls(b.statements.get(i), routerName, validIds)
            }
        }
        JsNodeKind.If => {
            const s = node as *mut JsIf
            converter.router_validate_calls(s.condition, routerName, validIds)
            converter.router_validate_calls(s.thenBlock, routerName, validIds)
            converter.router_validate_calls(s.elseBlock, routerName, validIds)
        }
        JsNodeKind.JSXElement => {
            const el = node as *mut JsJSXElement
            for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                converter.router_validate_calls(el.opening.attributes.get(i), routerName, validIds)
            }
            for(var i : uint = 0; i < el.children.size(); i++) {
                converter.router_validate_calls(el.children.get(i), routerName, validIds)
            }
        }
        JsNodeKind.JSXExpressionContainer => { converter.router_validate_calls((node as *mut JsJSXExpressionContainer).expression, routerName, validIds) }
        JsNodeKind.JSXAttribute => { converter.router_validate_calls((node as *mut JsJSXAttribute).value, routerName, validIds) }
        JsNodeKind.JSXFragment => {
            const f = node as *mut JsJSXFragment
            for(var i : uint = 0; i < f.children.size(); i++) {
                converter.router_validate_calls(f.children.get(i), routerName, validIds)
            }
        }
        default => {}
    }
}

// R3, R6, R7, R13: per-router structural validation.
func (converter : &mut JsConverter) router_validate(rd : *mut JsRouterDecl) {
    if(converter.diagnoser == null) return

    // R7: at most one fallback, and it must be the last route.
    var sawFallback = false
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.is_fallback) {
            sawFallback = true
        } else if(sawFallback) {
            var msg = std::string("fallback route must be the last route")
            converter.router_diag(&msg, r.decl_loc)
            break
        }
    }

    // R3: duplicate route ids within one router.
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const a = rd.routes.get(i)
        if(a == null || a.kind != JsNodeKind.RouteDecl) continue
        var ra = a as *mut JsRouteDecl
        for(var j : uint = i + 1; j < rd.routes.size(); j++) {
            const b = rd.routes.get(j)
            if(b == null || b.kind != JsNodeKind.RouteDecl) continue
            var rb = b as *mut JsRouteDecl
            if(ra.id.equals(&rb.id)) {
                var msg = std::string("route '#")
                msg.append_view(&ra.id)
                msg.append_view("' is declared twice in router \"")
                msg.append_view(&rd.name)
                msg.append_view("\"")
                converter.router_diag(&msg, rb.decl_loc)
            }
        }
    }

    // R6: each route body must render exactly one root element.
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(router_count_jsx_roots(r) != 1) {
            var msg = std::string("route body must render exactly one root element")
            converter.router_diag(&msg, r.decl_loc)
        }
    }

    // R13: mid-pattern wildcards are not supported (the only catch-all is `route *`).
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.is_url && r.pattern.contains(std::string_view("*"))) {
            var msg = std::string("unsupported route pattern '")
            msg.append_view(&r.pattern)
            msg.append_view("'")
            converter.router_diag(&msg, r.decl_loc)
        }
    }
}

// R4: duplicate router names in one component body.
func (converter : &mut JsConverter) router_validate_block(block : *mut JsBlock) {
    if(converter.diagnoser == null || block == null) return
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const a = block.statements.get(i)
        if(a == null || a.kind != JsNodeKind.RouterDecl) continue
        var ra = a as *mut JsRouterDecl
        for(var j : uint = i + 1; j < block.statements.size(); j++) {
            const b = block.statements.get(j)
            if(b == null || b.kind != JsNodeKind.RouterDecl) continue
            var rb = b as *mut JsRouterDecl
            if(ra.name.equals(&rb.name)) {
                var msg = std::string("router \"")
                msg.append_view(&rb.name)
                msg.append_view("\" is declared twice")
                converter.router_diag(&msg, rb.decl_loc)
            }
        }
    }
}

// ── server emission ────────────────────────────────────────────────────────

func (converter : &mut JsConverter) emit_router_server(rd : *mut JsRouterDecl) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()

    converter.router_validate(rd)

    // A router is a page singleton (D-2.4): guard the whole emission through the
    // page's component dedup map so rendering the declaring component twice does
    // not emit the registry/wrappers/stubs twice.
    const routerHash = rd.decl_loc as size_t
    var check = make_require_component_call_static(builder, &mut *converter.support, routerHash, location)
    var guard = builder.make_if_stmt(check, converter.parent, location)
    const guardBody = guard.get_body()
    guardBody.push(make_set_component_hash_call_static(builder, &mut *converter.support, routerHash, converter.parent, location))

    const outerVec = converter.vec
    converter.vec = guardBody

    // 1. The client runtime + hide rule, once per page (INV-18).
    converter.vec.push(converter.router_page_stmt(std::string_view("ensure_router_runtime")))

    // 1b. Server-side URL matching (§6.1): the generated function owns the
    // compile-time patterns, so it performs the match while it renders and
    // stores the selected id/params for `route_selected` and the activation tail.
    var matchSpec = std::string()
    router_match_spec(rd, &mut matchSpec)
    if(matchSpec.size() > 0 && converter.support.applyRouteUrlFn != null) {
        const fnNode = converter.support.applyRouteUrlFn
        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
        var fnId = builder.make_identifier(std::string_view("apply_route_url"), fnNode, false, location)
        var matchCall = builder.make_function_call_node(fnId, converter.parent, location)
        matchCall.get_args().push(pageId)
        matchCall.get_args().push(converter.router_string_value(rd.name))
        matchCall.get_args().push(converter.router_string_value(matchSpec.to_view()))
        converter.vec.push(matchCall)
    }

    // 2. Registry object + method table (template 1).
    converter.emit_router_registry(rd.name)

    const defaultId = router_default_id(rd)

    // 3. URL match table (template 3), emitted only when URL routes exist. The
    // client matcher (`$__uni_match_url`) scans it in order for Link clicks and
    // popstate; precedence is declaration order (a straight first-match scan).
    var hasUrl = false
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        if((rn as *mut JsRouteDecl).is_url) hasUrl = true
    }
    if(hasUrl) {
        var table = std::string()
        table.append_view("window.$__uni_routers[\"")
        router_js_escape(rd.name, &mut table)
        table.append_view("\"].table = { base: \"\", routes: [")
        var firstEntry = true
        const order = router_url_order(rd)
        for(var k : size_t = 0; k < order.size(); k++) {
            var r = rd.routes.get(order.get(k)) as *mut JsRouteDecl
            if(!firstEntry) table.append_view(", ")
            firstEntry = false
            if(r.is_fallback) {
                table.append_view("{ fallback: true, id: \"")
                router_js_escape(r.id, &mut table)
                table.append_view("\" }")
            } else {
                table.append_view("{ pattern: [")
                router_pattern_segments_js(r.pattern, &mut table)
                table.append_view("], id: \"")
                router_js_escape(r.id, &mut table)
                table.append_view("\" }")
            }
        }
        table.append_view("] };\n")
        converter.router_emit_js(&table)
    }

    // 4. One wrapper + stub per route.
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        converter.emit_route_server(rd.name, rn as *mut JsRouteDecl, defaultId)
    }

    // 4. Route manifest entries (static-export `routes.json`, §13.2.3).
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        var call = converter.router_page_stmt(std::string_view("add_route_pattern"))
        call.get_args().push(converter.router_string_value(rd.name))
        call.get_args().push(converter.router_string_value(r.pattern))
        call.get_args().push(converter.router_string_value(r.id))
        call.get_args().push(builder.make_bool_value(r.is_fallback, location))
        converter.vec.push(call)
    }

    // 4b. Per-route `<title>` for the selected route, plus 404 `noindex`
    // (§13.2.1/§13.2.2). Both are server-only head emissions.
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.title.size() == 0) continue
        var titleCall = converter.router_page_stmt(std::string_view("emit_route_title"))
        titleCall.get_args().push(converter.router_string_value(rd.name))
        titleCall.get_args().push(converter.router_string_value(r.id))
        titleCall.get_args().push(converter.router_string_value(defaultId))
        titleCall.get_args().push(converter.router_string_value(r.title))
        converter.vec.push(titleCall)
    }
    converter.vec.push(converter.router_page_stmt(std::string_view("emit_route_noindex")))

    // 5. `preload` routes hydrate at load (off the interaction path, §5.1).
    var preloadJs = std::string()
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.mode.equals(std::string_view("preload"))) {
            preloadJs.append_view("window.$__uni_preload(\"")
            router_js_escape(rd.name, &mut preloadJs)
            preloadJs.append_view("\", \"")
            router_js_escape(r.id, &mut preloadJs)
            preloadJs.append_view("\");\n")
        }
    }
    converter.router_emit_js(&preloadJs)

    // 6. Activation tail (pageJsEnd, after $__universal_flush()). For URL
    // routers, write the server's mount base into the client table first (§Q42).
    if(hasUrl) {
        var baseCall = converter.router_page_stmt(std::string_view("append_router_table_base"))
        baseCall.get_args().push(converter.router_string_value(rd.name))
        converter.vec.push(baseCall)
    }
    var tail = converter.router_page_stmt(std::string_view("append_router_initial_activation"))
    tail.get_args().push(converter.router_string_value(rd.name))
    tail.get_args().push(converter.router_string_value(defaultId))
    tail.get_args().push(builder.make_bool_value(hasUrl, location))
    converter.vec.push(tail)

    converter.vec = outerVec
    converter.vec.push(guard)
}

// The direct nested `route` declarations of a route body (Phase 6). Empty when
// the route has none.
func router_route_nested(route : *mut JsRouteDecl) : std::vector<*mut JsNode> {
    var out = std::vector<*mut JsNode>()
    if(route.body == null || route.body.kind != JsNodeKind.Block) return out
    const block = route.body as *mut JsBlock
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i)
        if(stmt != null && stmt.kind == JsNodeKind.RouteDecl) out.push(stmt)
    }
    return out
}

// The default id of a route list (declared `default`, else the first non-fallback).
func router_routes_default(routes : &std::vector<*mut JsNode>) : std::string_view {
    var first = std::string_view()
    for(var i : uint = 0; i < routes.size(); i++) {
        var r = routes.get(i) as *mut JsRouteDecl
        if(r.is_fallback) continue
        if(r.is_default) return r.id
        if(first.size() == 0) first = r.id
    }
    return first
}

// Emits the registry object + method table for a router name (template 1).
func (converter : &mut JsConverter) emit_router_registry(name : std::string_view) {
    var reg = std::string()
    reg.append_view("window.$__uni_routers[\"")
    router_js_escape(name, &mut reg)
    reg.append_view("\"] = { name: \"")
    router_js_escape(name, &mut reg)
    reg.append_view("\", currentRoute: null, routes: Object.create(null), table: null, $current: window.$_us(null), $url: window.$_us(null), $query: window.$_us(null) };\nObject.assign(window.$__uni_routers[\"")
    router_js_escape(name, &mut reg)
    reg.append_view("\"], window.$__uni_router_methods(\"")
    router_js_escape(name, &mut reg)
    reg.append_view("\"));\n")
    converter.router_emit_js(&reg)
}

// Expands the current outlet context: emits the nested router's wrappers and
// stubs at the `<Outlet />` position. Idempotent (one outlet per route).
func (converter : &mut JsConverter) emit_nested_routes() {
    if(converter.router_outlet_emitted) return
    converter.router_outlet_emitted = true
    const name = converter.router_outlet_name
    const def = converter.router_outlet_default
    for(var i : uint = 0; i < converter.router_outlet_routes.size(); i++) {
        var r = converter.router_outlet_routes.get(i) as *mut JsRouteDecl
        converter.emit_route_server(name, r, def)
    }
}

func (converter : &mut JsConverter) emit_route_server(routerName : std::string_view, route : *mut JsRouteDecl,
                                                      defaultId : std::string_view) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()

    // Nested routes (Phase 6): this route owns a nested router whose wrappers
    // render at its `<Outlet />`. The nested registry is emitted before the body
    // so its stubs (emitted during body conversion) can resolve it.
    const nested = router_route_nested(route)
    var nestedName = std::string()
    var nestedDefault = std::string_view()
    if(nested.size() > 0) {
        nestedName.append_view(&routerName)
        nestedName.append('#')
        nestedName.append_view(&route.id)
        nestedDefault = router_routes_default(&nested)
        converter.emit_router_registry(nestedName.to_view())
    }

    const root = router_route_root(route)
    var compName = std::string()
    // A route with nested children is a static layout; its client interactivity
    // would need a generated wrapper that renders the outlet. The nested level
    // remains independently hydrated, which is the layout-preservation property.
    if(nested.size() == 0) {
        compName = router_route_comp(root)
    }

    // Wrapper open tag. The route's own markup lives inside the boundary span,
    // never on the wrapper, so no prop can clobber the router attributes
    // (§2.3). `tabindex="-1"` is required for focus on activation (G-8).
    var open = std::string()
    open.append_view("<div class=\"chx-route\" id=\"r")
    open.append_uinteger(route.decl_loc)
    open.append_view("\" data-uni-route=\"")
    router_html_escape(routerName, &mut open)
    open.append('#')
    router_html_escape(route.id, &mut open)
    open.append_view("\" data-uni-route-active=\"")
    converter.router_emit_target(&open)

    // Selected route renders visible (no flash); the client activation tail is
    // the single source of truth for later navigation.
    var cond = converter.router_page_value(std::string_view("route_selected"))
    cond.get_args().push(converter.router_string_value(routerName))
    cond.get_args().push(converter.router_string_value(route.id))
    cond.get_args().push(converter.router_string_value(defaultId))
    var activeIf = builder.make_if_stmt(cond, converter.parent, location)
    const savedVec = converter.vec
    converter.vec = activeIf.get_body()
    converter.str.append_view("true")
    converter.put_chain_in()
    converter.vec = activeIf.add_else_body()
    converter.str.append_view("false")
    converter.put_chain_in()
    converter.vec = savedVec
    converter.vec.push(activeIf)

    var closeOpen = std::string()
    closeOpen.append_view("\" tabindex=\"-1\"><span data-chx-i id=\"u")
    closeOpen.append_uinteger(route.decl_loc)
    closeOpen.append_view("\">")
    converter.router_emit_target(&closeOpen)

    const isRemote = route.mode.equals(std::string_view("remote"))
    if(root != null && !isRemote) {
        // Set the outlet context across the body conversion (save/restore for
        // recursive nesting). Copy the routes vector; do not move the field.
        var prevRoutes = std::vector<*mut JsNode>()
        for(var pi : uint = 0; pi < converter.router_outlet_routes.size(); pi++) {
            prevRoutes.push(converter.router_outlet_routes.get(pi))
        }
        const prevName = converter.router_outlet_name
        const prevDefault = converter.router_outlet_default
        const prevEmitted = converter.router_outlet_emitted
        if(nested.size() > 0) {
            converter.router_outlet_routes = std::vector<*mut JsNode>()
            for(var ni : uint = 0; ni < nested.size(); ni++) {
                converter.router_outlet_routes.push(nested.get(ni))
            }
            converter.router_outlet_name = builder.allocate_view(nestedName.to_view())
            converter.router_outlet_default = builder.allocate_view(&nestedDefault)
            converter.router_outlet_emitted = false
        }
        converter.router_inject_param_props(route, root)
        converter.convertJsNode(root)
        // No `<Outlet />` in the layout: fall back to appending the nested
        // wrappers after the layout so children are never silently dropped.
        if(nested.size() > 0 && !converter.router_outlet_emitted) {
            converter.emit_nested_routes()
        }
        converter.router_outlet_routes = std::vector<*mut JsNode>()
        for(var ri : uint = 0; ri < prevRoutes.size(); ri++) {
            converter.router_outlet_routes.push(prevRoutes.get(ri))
        }
        converter.router_outlet_name = prevName
        converter.router_outlet_default = prevDefault
        converter.router_outlet_emitted = prevEmitted
    }

    var closeTag = std::string()
    closeTag.append_view("</span></div>")
    converter.router_emit_target(&closeTag)

    // Registration stub (template 2).
    var beforeHook = router_find_hook(route, std::string_view("onBeforeActivate"))
    var activateHook = router_find_hook(route, std::string_view("onActivate"))
    var deactivateHook = router_find_hook(route, std::string_view("onDeactivate"))

    converter.put_chain_in()
    const prevTarget = converter.target
    converter.target = BufferType.JavaScript
    var stub = &mut converter.str
    stub.append_view("window.$__uni_route_register(\"")
    router_js_escape(routerName, stub)
    stub.append_view("\", \"")
    router_js_escape(route.id, stub)
    stub.append_view("\", { key: \"")
    router_js_escape(routerName, stub)
    stub.append('#')
    router_js_escape(route.id, stub)
    stub.append_view("\", id: \"")
    router_js_escape(route.id, stub)
    stub.append_view("\", comp: ")
    if(compName.size() > 0) { stub.append_view(compName.to_view()) } else { stub.append_view("null") }
    stub.append_view(", baseProps: {")
    router_pattern_params_js(route.pattern, stub)
    stub.append_view("}, wrapperId: \"r")
    stub.append_uinteger(route.decl_loc)
    stub.append_view("\", hostId: \"u")
    stub.append_uinteger(route.decl_loc)
    stub.append_view("\", remote: ")
    if(isRemote) { stub.append_view("true") } else { stub.append_view("false") }
    stub.append_view(", isUrl: ")
    if(route.is_url) { stub.append_view("true") } else { stub.append_view("false") }
    stub.append_view(", fetchUrl: null, noscroll: ")
    if(route.noscroll) { stub.append_view("true") } else { stub.append_view("false") }
    stub.append_view(", title: ")
    if(route.title.size() > 0) {
        stub.append('"')
        router_js_escape(route.title, stub)
        stub.append('"')
    } else {
        stub.append_view("null")
    }
    stub.append_view(", nested: ")
    if(nested.size() > 0) {
        stub.append('"')
        router_js_escape(nestedName.to_view(), stub)
        stub.append('"')
    } else {
        stub.append_view("null")
    }
    stub.append_view(", nestedDefault: ")
    if(nestedDefault.size() > 0) {
        stub.append('"')
        router_js_escape(nestedDefault, stub)
        stub.append('"')
    } else {
        stub.append_view("null")
    }
    stub.append_view(", beforeActivate: ")
    if(beforeHook != null) { converter.convertJsNode(beforeHook) } else { stub.append_view("null") }
    stub.append_view(", onActivate: ")
    if(activateHook != null) { converter.convertJsNode(activateHook) } else { stub.append_view("null") }
    stub.append_view(", onDeactivate: ")
    if(deactivateHook != null) { converter.convertJsNode(deactivateHook) } else { stub.append_view("null") }
    stub.append_view(" });\n")
    converter.put_chain_in()
    converter.target = prevTarget
}
