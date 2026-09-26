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

// One entry in the emitted URL match table / server spec (§6.4 nested URL
// ownership). A top-level route has an empty chain; a nested URL route's `id` is
// the *root* layout route to activate and `chainRegs`/`chainIds` walk from the
// root down to the leaf. `pattern` is the full accumulated pattern, so the
// matcher stays a single exact scan on both server and client.
public struct RouterUrlEntry {
    var id : std::string_view
    var pattern : std::string_view
    var is_fallback : bool
    var prefix : bool
    var chainRegs : std::vector<std::string_view>
    var chainIds : std::vector<std::string_view>
    var order : int
    var decl_loc : ubigint
}

// True when two patterns have the same shape: same segment count and, per
// segment, both literal-and-equal or both a `{param}`. Such patterns can match
// the same path, so only one is reachable (R9).
func router_pattern_shape_equal(a : std::string_view, b : std::string_view) : bool {
    var ia : size_t = 0
    var ib : size_t = 0
    while(true) {
        while(ia < a.size() && a.get(ia) == '/') { ia = ia + 1 }
        while(ib < b.size() && b.get(ib) == '/') { ib = ib + 1 }
        const aEnd = ia >= a.size()
        const bEnd = ib >= b.size()
        if(aEnd || bEnd) { return aEnd && bEnd }
        var ja = ia
        while(ja < a.size() && a.get(ja) != '/') { ja = ja + 1 }
        var jb = ib
        while(jb < b.size() && b.get(jb) != '/') { jb = jb + 1 }
        const sa = a.subview(ia, ja)
        const sb = b.subview(ib, jb)
        const pa = sa.size() >= 2 && sa.get(0) == '{' && sa.get(sa.size() - 1) == '}'
        const pb = sb.size() >= 2 && sb.get(0) == '{' && sb.get(sb.size() - 1) == '}'
        if(pa != pb) { return false }
        if(!pa && !sa.equals(&sb)) { return false }
        ia = ja
        ib = jb
    }
    return false
}

// R9: two URL patterns with the same shape are ambiguous (only the first can
// ever match). Checked over the flattened entries, so nested patterns count too.
func (converter : &mut JsConverter) router_validate_ambiguity(entries : &std::vector<*mut RouterUrlEntry>) {
    if(converter.diagnoser == null) return
    for(var i : size_t = 0; i < entries.size(); i++) {
        var a = entries.get(i)
        if(a.is_fallback || a.prefix) { continue }
        for(var j : size_t = i + 1; j < entries.size(); j++) {
            var b = entries.get(j)
            if(b.is_fallback || b.prefix) { continue }
            if(!router_pattern_shape_equal(a.pattern, b.pattern)) { continue }
            var msg = std::string("route patterns '")
            msg.append_view(&a.pattern)
            msg.append_view("' and '")
            msg.append_view(&b.pattern)
            msg.append_view("' are ambiguous")
            converter.router_diag(&msg, b.decl_loc)
        }
    }
}

// Precedence (D-6.2/D-6.9): more literal segments → fewer params → declaration
// order; the fallback is always last.
func router_entry_precedes(a : *mut RouterUrlEntry, b : *mut RouterUrlEntry) : bool {
    if(a.is_fallback != b.is_fallback) { return !a.is_fallback }
    if(a.is_fallback) { return a.order < b.order }
    // Prefix (nested-fallback) entries are scanned after every exact entry.
    if(a.prefix != b.prefix) { return !a.prefix }
    var la = 0
    var pa = 0
    router_pattern_score(a.pattern, &mut la, &mut pa)
    var lb = 0
    var pb = 0
    router_pattern_score(b.pattern, &mut lb, &mut pb)
    if(la != lb) { return la > lb }
    if(pa != pb) { return pa < pb }
    return a.order < b.order
}

// Depth-first walk of the route tree, emitting one `RouterUrlEntry` per URL
// route. `ownerRegistry` owns `routes` in the client registry; `rootId` is the
// top-level route every entry activates first (the layout); `pathRegs`/`pathIds`
// are the chain steps from the root to `ownerRegistry` (empty at the top level).
func (converter : &mut JsConverter) router_collect_url_entries(routes : &std::vector<*mut JsNode>,
        ownerRegistry : std::string_view, rootRegistry : std::string_view, rootId : std::string_view,
        pathRegs : &std::vector<std::string_view>, pathIds : &std::vector<std::string_view>,
        inherited : std::string_view, out : &mut std::vector<*mut RouterUrlEntry>, order : &mut int) {
    const builder = converter.builder
    const isTop = ownerRegistry.equals(&rootRegistry)

    for(var i : uint = 0; i < routes.size(); i++) {
        var r = routes.get(i) as *mut JsRouteDecl

        if(isTop && r.is_fallback) {
            const fe = builder.allocate<RouterUrlEntry>()
            new (fe) RouterUrlEntry {
                id : builder.allocate_view(&r.id),
                pattern : std::string_view(),
                is_fallback : true,
                prefix : false,
                chainRegs : std::vector<std::string_view>(),
                chainIds : std::vector<std::string_view>(),
                order : *order,
                decl_loc : r.decl_loc
            }
            *order = *order + 1
            out.push(fe)
            continue
        }
        if(r.is_fallback) { continue }

        var full = std::string()
        full.append_view(&inherited)
        full.append_view(&r.pattern)
        const fullPtr = builder.allocate_str(full.data(), full.size())
        const fullView = std::string_view(fullPtr, full.size())

        var selfRegs = std::vector<std::string_view>()
        var selfIds = std::vector<std::string_view>()
        for(var k : uint = 0; k < pathRegs.size(); k++) {
            selfRegs.push(pathRegs.get(k))
            selfIds.push(pathIds.get(k))
        }
        if(!isTop) {
            var regStr = std::string()
            regStr.append_view(&ownerRegistry)
            const regPtr = builder.allocate_str(regStr.data(), regStr.size())
            selfRegs.push(std::string_view(regPtr, regStr.size()))
            selfIds.push(builder.allocate_view(&r.id))
        }

        if(r.is_url) {
            var entryRegs = std::vector<std::string_view>()
            var entryIds = std::vector<std::string_view>()
            for(var k : uint = 0; k < selfRegs.size(); k++) {
                entryRegs.push(selfRegs.get(k))
                entryIds.push(selfIds.get(k))
            }
            const e = builder.allocate<RouterUrlEntry>()
            new (e) RouterUrlEntry {
                id : builder.allocate_view(&rootId),
                pattern : fullView,
                is_fallback : false,
                prefix : false,
                chainRegs : entryRegs,
                chainIds : entryIds,
                order : *order,
                decl_loc : r.decl_loc
            }
            *order = *order + 1
            out.push(e)
        }

        const nested = router_route_nested(r)
        if(nested.size() > 0) {
            var childReg = std::string()
            childReg.append_view(&ownerRegistry)
            childReg.append('#')
            childReg.append_view(&r.id)
            const childPtr = builder.allocate_str(childReg.data(), childReg.size())
            const childView = std::string_view(childPtr, childReg.size())

            // Nested `route *` fallback: a prefix entry on this route's full
            // pattern, activating the layout and then the nested fallback, so an
            // unknown remainder under the layout resolves instead of 404-ing.
            var nestedFb = router_nested_fallback(&nested)
            if(r.is_url && nestedFb != null) {
                var fbRegs = std::vector<std::string_view>()
                var fbIds = std::vector<std::string_view>()
                for(var k : uint = 0; k < selfRegs.size(); k++) {
                    fbRegs.push(selfRegs.get(k))
                    fbIds.push(selfIds.get(k))
                }
                fbRegs.push(childView)
                fbIds.push(builder.allocate_view(&std::string_view("*")))
                const fe = builder.allocate<RouterUrlEntry>()
                new (fe) RouterUrlEntry {
                    id : builder.allocate_view(&rootId),
                    pattern : fullView,
                    is_fallback : false,
                    prefix : true,
                    chainRegs : fbRegs,
                    chainIds : fbIds,
                    order : *order,
                    decl_loc : nestedFb.decl_loc
                }
                *order = *order + 1
                out.push(fe)
            }

            converter.router_collect_url_entries(&nested, childView, rootRegistry, rootId,
                                                 &selfRegs, &selfIds, fullView, out, order)
        }
    }
}

// The `route *` fallback of a nested route list, or null.
func router_nested_fallback(routes : &std::vector<*mut JsNode>) : *mut JsRouteDecl {
    for(var i : uint = 0; i < routes.size(); i++) {
        var r = routes.get(i) as *mut JsRouteDecl
        if(r.is_fallback) { return r }
    }
    return null
}

// Builds the sorted URL entry list for a router declaration.
func (converter : &mut JsConverter) router_url_entries(rd : *mut JsRouterDecl) : std::vector<*mut RouterUrlEntry> {
    const builder = converter.builder
    var out = std::vector<*mut RouterUrlEntry>()
    var order = 0
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var one = std::vector<*mut JsNode>()
        one.push(rn)
        var emptyRegs = std::vector<std::string_view>()
        var emptyIds = std::vector<std::string_view>()
        converter.router_collect_url_entries(&one, rd.name, rd.name, (rn as *mut JsRouteDecl).id,
                                             &emptyRegs, &emptyIds, std::string_view(""), &mut out, &mut order)
    }
    // insertion sort (pointers; stable via the `order` tie-break)
    for(var a : size_t = 1; a < out.size(); a++) {
        const key = out.get(a)
        var b = a
        while(b > 0 && router_entry_precedes(key, out.get(b - 1))) {
            *out.get_ref(b) = out.get(b - 1)
            b = b - 1
        }
        *out.get_ref(b) = key
    }
    return out
}

// Encodes the activation chain for the server spec: `reg` US `id` (RS between
// steps). The converter and `router::parse_route_chain` own this format.
func router_chain_field(entry : *mut RouterUrlEntry, out : &mut std::string) {
    const rec = std::string_view("\x1e")
    const unit = std::string_view("\x1f")
    for(var i : uint = 0; i < entry.chainRegs.size(); i++) {
        if(i > 0) { out.append_view(&rec) }
        const reg = entry.chainRegs.get(i)
        out.append_view(&reg)
        out.append_view(&unit)
        const cid = entry.chainIds.get(i)
        out.append_view(&cid)
    }
}

// Builds the compact server match spec consumed by `apply_route_url`:
// `<id>\t<pattern>\t<is_fallback>\t<chain>\n`, in precedence order.
func router_match_spec(entries : &std::vector<*mut RouterUrlEntry>, out : &mut std::string) {
    for(var k : size_t = 0; k < entries.size(); k++) {
        var e = entries.get(k)
        out.append_view(&e.id)
        out.append('\t')
        out.append_view(&e.pattern)
        out.append('\t')
        if(e.is_fallback) { out.append('1') }
        else if(e.prefix) { out.append('2') }
        else { out.append('0') }
        out.append('\t')
        router_chain_field(e, out)
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

// Injects one synthetic SSR attribute per `{param}` in `effectivePattern` into a
// route body's JSX root (D-2.7). `effectivePattern` accumulates the ancestor
// patterns for nested routes, so a nested id route under `/projects/{id}` still
// receives `id`. SSR-only: the route body root is never client-emitted.
func (converter : &mut JsConverter) router_inject_param_props(effectivePattern : std::string_view, root : *mut JsNode) {
    if(root == null || root.kind != JsNodeKind.JSXElement) return
    const names = router_pattern_param_names(effectivePattern)
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
// key must exist so hydration is never handed a missing prop. `firstIn` lets a
// caller prepend other compile-time props without a leading separator.
func router_pattern_params_js(pattern : std::string_view, out : &mut std::string, firstIn : bool) {
    var first = firstIn
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

// R10 is a warning (the page still renders; it just renders inert without a
// server parameter), so it goes through the CBI warning channel.
func (converter : &mut JsConverter) router_warn(msg : &std::string, loc : ubigint) {
    if(converter.diagnoser == null) return
    converter.diagnoser.warning(&msg.to_view(), loc)
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
                                                         validIds : &std::vector<*mut JsNode>, loc : ubigint) {
    if(node == null || converter.diagnoser == null) return
    switch(node.kind) {
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall
            if(call.callee != null && call.callee.kind == JsNodeKind.MemberAccess) {
                const mem = call.callee as *mut JsMemberAccess
                const prop = mem.property
                if(prop.equals(std::string_view("activateRoute")) || prop.equals(std::string_view("preload")) ||
                   prop.equals(std::string_view("buildPath")) || prop.equals(std::string_view("replaceRoute"))) {
                    // `router("name").<verb>("literal-id")`
                    if(mem.object != null && mem.object.kind == JsNodeKind.FunctionCall) {
                        const recv = mem.object as *mut JsFunctionCall
                        if(recv.callee != null && recv.callee.kind == JsNodeKind.Identifier &&
                           (recv.callee as *mut JsIdentifier).value.equals(std::string_view("router")) &&
                           recv.args.size() > 0 && recv.args.get(0).kind == JsNodeKind.Literal &&
                           call.args.size() > 0 && call.args.get(0).kind == JsNodeKind.Literal) {
                            const nameLit = (recv.args.get(0) as *mut JsLiteral).value
                            const idLit = (call.args.get(0) as *mut JsLiteral).value
                            var nameName = nameLit
                            if(nameName.size() >= 2 && nameName.get(0) == '"' && nameName.get(nameName.size() - 1) == '"') {
                                nameName = nameName.subview(1, nameName.size() - 1)
                            }
                            // Only validate the router this component declares.
                            if(nameName.equals(&routerName)) {
                                var found = false
                                for(var i : uint = 0; i < validIds.size(); i++) {
                                    var r = validIds.get(i) as *mut JsRouteDecl
                                    var quoted = std::string("\"")
                                    quoted.append_view(&r.id)
                                    quoted.append('"')
                                    if(idLit.equals(quoted.to_view())) { found = true }
                                }
                                if(!found) {
                                    // Strip the JS string quotes to match the
                                    // frozen message (single-quoted id).
                                    var idName = idLit
                                    if(idName.size() >= 2 && idName.get(0) == '"' && idName.get(idName.size() - 1) == '"') {
                                        idName = idName.subview(1, idName.size() - 1)
                                    }
                                    var msg = std::string("no route '")
                                    msg.append_view(&idName)
                                    msg.append_view("' in router \"")
                                    msg.append_view(&routerName)
                                    msg.append_view("\"")
                                    converter.router_diag(&msg, loc)
                                }
                            }
                        }
                    }
                }
            }
            for(var i : uint = 0; i < call.args.size(); i++) {
                converter.router_validate_calls(call.args.get(i), routerName, validIds, loc)
            }
            converter.router_validate_calls(call.callee, routerName, validIds, loc)
        }
        JsNodeKind.ExpressionStatement => { converter.router_validate_calls((node as *mut JsExpressionStatement).expression, routerName, validIds, loc) }
        JsNodeKind.Return => { converter.router_validate_calls((node as *mut JsReturn).value, routerName, validIds, loc) }
        JsNodeKind.VarDecl => { converter.router_validate_calls((node as *mut JsVarDecl).value, routerName, validIds, loc) }
        JsNodeKind.ArrowFunction => { converter.router_validate_calls((node as *mut JsArrowFunction).body, routerName, validIds, loc) }
        JsNodeKind.Block => {
            const b = node as *mut JsBlock
            for(var i : uint = 0; i < b.statements.size(); i++) {
                converter.router_validate_calls(b.statements.get(i), routerName, validIds, loc)
            }
        }
        JsNodeKind.If => {
            const s = node as *mut JsIf
            converter.router_validate_calls(s.condition, routerName, validIds, loc)
            converter.router_validate_calls(s.thenBlock, routerName, validIds, loc)
            converter.router_validate_calls(s.elseBlock, routerName, validIds, loc)
        }
        JsNodeKind.JSXElement => {
            const el = node as *mut JsJSXElement
            for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                converter.router_validate_calls(el.opening.attributes.get(i), routerName, validIds, loc)
            }
            for(var i : uint = 0; i < el.children.size(); i++) {
                converter.router_validate_calls(el.children.get(i), routerName, validIds, loc)
            }
        }
        JsNodeKind.JSXExpressionContainer => { converter.router_validate_calls((node as *mut JsJSXExpressionContainer).expression, routerName, validIds, loc) }
        JsNodeKind.JSXAttribute => { converter.router_validate_calls((node as *mut JsJSXAttribute).value, routerName, validIds, loc) }
        JsNodeKind.JSXFragment => {
            const f = node as *mut JsJSXFragment
            for(var i : uint = 0; i < f.children.size(); i++) {
                converter.router_validate_calls(f.children.get(i), routerName, validIds, loc)
            }
        }
        default => {}
    }
}

// R3, R6, R7, R8, R13, R14: structural validation for a router declaration and,
// recursively, every nested router (a route body's nested `route` children form
// a sub-router under a derived name).
func (converter : &mut JsConverter) router_validate(rd : *mut JsRouterDecl) {
    converter.router_validate_routes(&rd.routes, rd.name, std::string_view(""))
}

func (converter : &mut JsConverter) router_validate_routes(routes : &std::vector<*mut JsNode>, routerName : std::string_view,
                                                            inheritedPattern : std::string_view) {
    if(converter.diagnoser == null) return

    // R10: a router with neither a declared `default` nor a `*` fallback renders
    // inert until a server parameter selects a route. Warning, not an error.
    var hasAny = false
    var hasDefault = false
    var hasFallback = false
    var firstLoc : ubigint = 0
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(!hasAny) { firstLoc = r.decl_loc }
        hasAny = true
        if(r.is_default) { hasDefault = true }
        if(r.is_fallback) { hasFallback = true }
    }
    if(hasAny && !hasDefault && !hasFallback) {
        var msg = std::string("router \"")
        msg.append_view(&routerName)
        msg.append_view("\" has no default route; the page renders inert without a server parameter")
        converter.router_warn(&msg, firstLoc)
    }

    // R7: at most one fallback, and it must be the last route.
    var sawFallback = false
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
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
    for(var i : uint = 0; i < routes.size(); i++) {
        const a = routes.get(i)
        if(a == null || a.kind != JsNodeKind.RouteDecl) continue
        var ra = a as *mut JsRouteDecl
        for(var j : uint = i + 1; j < routes.size(); j++) {
            const b = routes.get(j)
            if(b == null || b.kind != JsNodeKind.RouteDecl) continue
            var rb = b as *mut JsRouteDecl
            if(ra.id.equals(&rb.id)) {
                var msg = std::string("route '#")
                msg.append_view(&ra.id)
                msg.append_view("' is declared twice in router \"")
                msg.append_view(&routerName)
                msg.append_view("\"")
                converter.router_diag(&msg, rb.decl_loc)
            }
        }
    }

    // R6: each route body must render exactly one root element.
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(router_count_jsx_roots(r) != 1) {
            var msg = std::string("route body must render exactly one root element")
            converter.router_diag(&msg, r.decl_loc)
        }
    }

    // R13: mid-pattern wildcards are not supported (the only catch-all is `route *`).
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(r.is_url && r.pattern.contains(std::string_view("*"))) {
            var msg = std::string("unsupported route pattern '")
            msg.append_view(&r.pattern)
            msg.append_view("'")
            converter.router_diag(&msg, r.decl_loc)
        }
    }

    // R14: route bodies and hooks must not reach `$__uni_*` internals.
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        if(router_scan_internals(r.body)) {
            var msg = std::string("route bodies cannot call runtime internals")
            converter.router_diag(&msg, r.decl_loc)
        }
        for(var h : uint = 0; h < r.hooks.size(); h++) {
            const hook = r.hooks.get(h) as *mut JsRouteHook
            if(router_scan_internals(hook.fn)) {
                var msg = std::string("route bodies cannot call runtime internals")
                converter.router_diag(&msg, r.decl_loc)
            }
        }
    }

    // R8: literal ids passed to `router("m").activateRoute("x")` etc. must be
    // declared in this router.
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        converter.router_validate_calls(r.body, routerName, routes, r.decl_loc)
        for(var h : uint = 0; h < r.hooks.size(); h++) {
            const hook = r.hooks.get(h) as *mut JsRouteHook
            converter.router_validate_calls(hook.fn, routerName, routes, r.decl_loc)
        }
    }

    // R11/R12: route-component prop reads (design §14.8).
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        var ep = std::string()
        ep.append_view(&inheritedPattern)
        ep.append_view(&r.pattern)
        converter.router_validate_props(r, ep.to_view())
    }

    // Recurse: nested routes form a sub-router under a derived name.
    for(var i : uint = 0; i < routes.size(); i++) {
        const rn = routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        var r = rn as *mut JsRouteDecl
        const nested = router_route_nested(r)
        if(nested.size() == 0) { continue }
        var nestedName = std::string()
        nestedName.append_view(&routerName)
        nestedName.append('#')
        nestedName.append_view(&r.id)
        var childPattern = std::string()
        childPattern.append_view(&inheritedPattern)
        childPattern.append_view(&r.pattern)
        converter.router_validate_routes(&nested, nestedName.to_view(), childPattern.to_view())
    }
}

// ── R11/R12: route-component prop validation (design §14.8) ─────────────────
//
// A route root component receives only the route root's compile-time attributes
// plus the `{param}` values injected by the router; every other `props.X` read is
// undefined at runtime (R11). A `dangerouslySetInnerHTML` fed one of those params
// is an XSS footgun (R12). The component's parsed JS body is reachable through
// `ComponentSignature.js_body` (set by the `#universal` macro).

func router_push_unique(out : &mut std::vector<std::string_view>, name : std::string_view) {
    for(var i : uint = 0; i < out.size(); i++) {
        if(out.get(i).equals(&name)) { return }
    }
    out.push(name)
}

// Walks a JS/JSX tree collecting `props.<name>` reads. `dangerousReads` receives
// the subset that appears under a `dangerouslySetInnerHTML` attribute.
func router_collect_prop_reads(node : *mut JsNode, propsName : std::string_view,
                               reads : &mut std::vector<std::string_view>,
                               dangerousReads : &mut std::vector<std::string_view>,
                               inDangerous : bool) {
    if(node == null) return
    switch(node.kind) {
        JsNodeKind.MemberAccess => {
            const m = node as *mut JsMemberAccess
            if(m.object != null && m.object.kind == JsNodeKind.Identifier) {
                const id = m.object as *mut JsIdentifier
                if(id.value.equals(&propsName) && !m.property.equals(std::string_view("children"))) {
                    router_push_unique(reads, m.property)
                    if(inDangerous) { router_push_unique(dangerousReads, m.property) }
                }
            }
            router_collect_prop_reads(m.object, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.FunctionCall => {
            const c = node as *mut JsFunctionCall
            router_collect_prop_reads(c.callee, propsName, reads, dangerousReads, inDangerous)
            for(var i : uint = 0; i < c.args.size(); i++) {
                router_collect_prop_reads(c.args.get(i), propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.ExpressionStatement => { router_collect_prop_reads((node as *mut JsExpressionStatement).expression, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.Block => {
            const b = node as *mut JsBlock
            for(var i : uint = 0; i < b.statements.size(); i++) {
                router_collect_prop_reads(b.statements.get(i), propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.If => {
            const s = node as *mut JsIf
            router_collect_prop_reads(s.condition, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.thenBlock, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.elseBlock, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.Return => { router_collect_prop_reads((node as *mut JsReturn).value, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.VarDecl => { router_collect_prop_reads((node as *mut JsVarDecl).value, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.ArrowFunction => { router_collect_prop_reads((node as *mut JsArrowFunction).body, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.BinaryOp => {
            const s = node as *mut JsBinaryOp
            router_collect_prop_reads(s.left, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.right, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.Ternary => {
            const s = node as *mut JsTernary
            router_collect_prop_reads(s.condition, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.consequent, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.alternate, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.UnaryOp => { router_collect_prop_reads((node as *mut JsUnaryOp).operand, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.Paren => { router_collect_prop_reads((node as *mut JsParen).expression, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.Spread => { router_collect_prop_reads((node as *mut JsSpread).argument, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.ArrayLiteral => {
            const a = node as *mut JsArrayLiteral
            for(var i : uint = 0; i < a.elements.size(); i++) {
                router_collect_prop_reads(a.elements.get(i), propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.ObjectLiteral => {
            const o = node as *mut JsObjectLiteral
            for(var i : uint = 0; i < o.properties.size(); i++) {
                router_collect_prop_reads(o.properties.get(i).value, propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.IndexAccess => {
            const s = node as *mut JsIndexAccess
            router_collect_prop_reads(s.object, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.index, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.JSXElement => {
            const el = node as *mut JsJSXElement
            for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                router_collect_prop_reads(el.opening.attributes.get(i), propsName, reads, dangerousReads, inDangerous)
            }
            for(var i : uint = 0; i < el.children.size(); i++) {
                router_collect_prop_reads(el.children.get(i), propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.JSXExpressionContainer => { router_collect_prop_reads((node as *mut JsJSXExpressionContainer).expression, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.JSXAttribute => {
            const attr = node as *mut JsJSXAttribute
            const childDangerous = inDangerous || attr.name.equals(std::string_view("dangerouslySetInnerHTML"))
            router_collect_prop_reads(attr.value, propsName, reads, dangerousReads, childDangerous)
        }
        JsNodeKind.JSXSpreadAttribute => { router_collect_prop_reads((node as *mut JsJSXSpreadAttribute).argument, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.JSXFragment => {
            const f = node as *mut JsJSXFragment
            for(var i : uint = 0; i < f.children.size(); i++) {
                router_collect_prop_reads(f.children.get(i), propsName, reads, dangerousReads, inDangerous)
            }
        }
        JsNodeKind.For => {
            const s = node as *mut JsFor
            router_collect_prop_reads(s.init, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.condition, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.update, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.body, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.ForIn => {
            const s = node as *mut JsForIn
            router_collect_prop_reads(s.left, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.right, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.body, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.ForOf => {
            const s = node as *mut JsForOf
            router_collect_prop_reads(s.left, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.right, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.body, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.While => {
            const s = node as *mut JsWhile
            router_collect_prop_reads(s.condition, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.body, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.DoWhile => {
            const s = node as *mut JsDoWhile
            router_collect_prop_reads(s.condition, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.body, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.Switch => {
            const s = node as *mut JsSwitch
            router_collect_prop_reads(s.discriminant, propsName, reads, dangerousReads, inDangerous)
            for(var i : uint = 0; i < s.cases.size(); i++) {
                const c = s.cases.get_ptr(i)
                router_collect_prop_reads(c.test, propsName, reads, dangerousReads, inDangerous)
                for(var j : uint = 0; j < c.body.size(); j++) {
                    router_collect_prop_reads(c.body.get(j), propsName, reads, dangerousReads, inDangerous)
                }
            }
        }
        JsNodeKind.TryCatch => {
            const s = node as *mut JsTryCatch
            router_collect_prop_reads(s.tryBlock, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.catchBlock, propsName, reads, dangerousReads, inDangerous)
            router_collect_prop_reads(s.finallyBlock, propsName, reads, dangerousReads, inDangerous)
        }
        JsNodeKind.Throw => { router_collect_prop_reads((node as *mut JsThrow).argument, propsName, reads, dangerousReads, inDangerous) }
        JsNodeKind.Yield => { router_collect_prop_reads((node as *mut JsYield).argument, propsName, reads, dangerousReads, inDangerous) }
        default => {}
    }
}

func router_is_pattern_param(pattern : std::string_view, name : std::string_view) : bool {
    const names = router_pattern_param_names(pattern)
    for(var i : uint = 0; i < names.size(); i++) {
        if(names.get(i).equals(&name)) { return true }
    }
    return false
}

// R11/R12 for one route.
func (converter : &mut JsConverter) router_validate_props(route : *mut JsRouteDecl,
                                                           effectivePattern : std::string_view) {
    if(converter.diagnoser == null) return
    const root = router_route_root(route)
    if(root == null || root.kind != JsNodeKind.JSXElement) return
    const rootEl = root as *mut JsJSXElement
    const sig = rootEl.componentSignature
    if(sig == null || sig.js_body == null) return

    // Allowed prop names: the params of the (accumulated) pattern, the route
    // root's declared attributes, and `children`.
    var allowed = std::vector<std::string_view>()
    const paramNames = router_pattern_param_names(effectivePattern)
    for(var i : uint = 0; i < paramNames.size(); i++) { allowed.push(paramNames.get(i)) }
    for(var i : uint = 0; i < rootEl.opening.attributes.size(); i++) {
        const attr = rootEl.opening.attributes.get(i)
        if(attr != null && attr.kind == JsNodeKind.JSXAttribute) {
            router_push_unique(&mut allowed, (attr as *mut JsJSXAttribute).name)
        }
    }
    router_push_unique(&mut allowed, std::string_view("children"))

    var reads = std::vector<std::string_view>()
    var dangerousReads = std::vector<std::string_view>()
    router_collect_prop_reads(sig.js_body as *mut JsNode, sig.propsName, &mut reads, &mut dangerousReads, false)

    for(var i : uint = 0; i < reads.size(); i++) {
        const name = reads.get(i)
        var ok = false
        for(var a : uint = 0; a < allowed.size(); a++) {
            if(allowed.get(a).equals(&name)) { ok = true }
        }
        if(!ok) {
            var msg = std::string("route prop '")
            msg.append_view(&name)
            msg.append_view("' is not declared: not an attribute of the route root and not a param of '")
            msg.append_view(&effectivePattern)
            msg.append_view("'")
            converter.router_diag(&msg, route.decl_loc)
        }
    }

    // R12: a `dangerouslySetInnerHTML` fed one of the route's params.
    for(var i : uint = 0; i < dangerousReads.size(); i++) {
        const name = dangerousReads.get(i)
        if(router_is_pattern_param(effectivePattern, name)) {
            var msg = std::string("route params must not be injected as raw HTML")
            converter.router_diag(&msg, route.decl_loc)
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

    // 1b. Server-side URL matching (§6.1/§6.4): the generated function owns the
    // compile-time patterns, so it performs the match while it renders and
    // stores the selected id/params (plus the nested activation chain) for
    // `route_selected` and the activation tail.
    const entries = converter.router_url_entries(rd)
    const hasUrl = entries.size() > 0
    converter.router_validate_ambiguity(&entries)
    var matchSpec = std::string()
    router_match_spec(&entries, &mut matchSpec)
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
    // popstate; it is already in precedence order (a straight first-match scan).
    // Nested URL routes appear here as full patterns with an activation chain,
    // so a single table drives the whole tree (server and client agree).
    if(hasUrl) {
        converter.router_emit_url_table(rd.name, &entries)
    }

    // 4. One wrapper + stub per route.
    for(var i : uint = 0; i < rd.routes.size(); i++) {
        const rn = rd.routes.get(i)
        if(rn == null || rn.kind != JsNodeKind.RouteDecl) continue
        converter.emit_route_server(rd.name, rn as *mut JsRouteDecl, defaultId, std::string_view(""))
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

    // 4b. 404 `noindex` (§13.2.2). Per-route `<title>` is emitted from
    // `emit_route_server` so nested routes get titles too (§13.2.1).
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

// Emits the client URL match table (template 3) for a router: one entry per
// precedence-ordered URL entry, each carrying the nested activation chain. The
// chain is omitted when empty to keep the common top-level case byte-identical.
func (converter : &mut JsConverter) router_emit_url_table(name : std::string_view,
                                                          entries : &std::vector<*mut RouterUrlEntry>) {
    var table = std::string()
    table.append_view("window.$__uni_routers[\"")
    router_js_escape(name, &mut table)
    table.append_view("\"].table = { base: \"\", routes: [")
    var firstEntry = true
    for(var k : size_t = 0; k < entries.size(); k++) {
        var e = entries.get(k)
        if(!firstEntry) table.append_view(", ")
        firstEntry = false
        if(e.is_fallback) {
            table.append_view("{ fallback: true, id: \"")
            router_js_escape(e.id, &mut table)
            table.append_view("\" }")
            continue
        }
        table.append_view("{ pattern: [")
        router_pattern_segments_js(e.pattern, &mut table)
        table.append_view("], id: \"")
        router_js_escape(e.id, &mut table)
        table.append_view("\"")
        if(e.prefix) { table.append_view(", prefix: true") }
        if(e.chainRegs.size() > 0) {
            table.append_view(", chain: [")
            for(var c : uint = 0; c < e.chainRegs.size(); c++) {
                if(c > 0) { table.append_view(", ") }
                table.append_view("[\"")
                router_js_escape(e.chainRegs.get(c), &mut table)
                table.append_view("\", \"")
                router_js_escape(e.chainIds.get(c), &mut table)
                table.append_view("\"]")
            }
            table.append_view("]")
        }
        table.append_view(" }")
    }
    table.append_view("] };\n")
    converter.router_emit_js(&table)
}

// Generates the anonymous client function for a native-rooted route layout, so a
// route with nested children is interactive. The body is converted in JavaScript
// mode (its `<Outlet />` becomes an opaque `__uni_outlet` boundary consumed by
// hydration); the SSR pass still expands the outlet into the nested wrappers.
func (converter : &mut JsConverter) emit_route_layout_client(route : *mut JsRouteDecl, root : *mut JsNode) : std::string {
    var name = std::string("$__uni_route_layout_")
    name.append_uinteger(route.decl_loc)
    const nameView = name.to_view()

    converter.put_chain_in()
    const prevTarget = converter.target
    converter.target = BufferType.JavaScript
    converter.str.append_view("window.")
    converter.str.append_view(&nameView)
    converter.str.append_view(" = function(props) { return ")
    converter.convertJsNode(root)
    converter.str.append_view("; };\n")
    converter.put_chain_in()
    converter.target = prevTarget

    return name
}

// Generates the anonymous client function returning a route root component's
// children as vnodes, so a layout component that renders `{props.children}` can
// be hydrated. Its `<Outlet />` becomes an opaque `__uni_outlet` boundary.
func (converter : &mut JsConverter) emit_route_children_client(route : *mut JsRouteDecl, root : *mut JsNode) : std::string {
    var name = std::string("$__uni_route_children_")
    name.append_uinteger(route.decl_loc)
    const nameView = name.to_view()
    const el = root as *mut JsJSXElement

    converter.put_chain_in()
    const prevTarget = converter.target
    converter.target = BufferType.JavaScript
    converter.str.append_view("window.")
    converter.str.append_view(&nameView)
    converter.str.append_view(" = function() { return [")
    for(var i : uint = 0; i < el.children.size(); i++) {
        if(i > 0) { converter.str.append_view(", ") }
        converter.convertJsNode(el.children.get(i))
    }
    converter.str.append_view("]; };\n")
    converter.put_chain_in()
    converter.target = prevTarget

    return name
}

// Expands the current outlet context: emits the nested router's wrappers and
// stubs at the `<Outlet />` position. Idempotent (one outlet per route).
func (converter : &mut JsConverter) emit_nested_routes() {
    if(converter.router_outlet_emitted) return
    converter.router_outlet_emitted = true
    const name = converter.router_outlet_name
    const def = converter.router_outlet_default
    const inherited = converter.router_outlet_inherited
    for(var i : uint = 0; i < converter.router_outlet_routes.size(); i++) {
        var r = converter.router_outlet_routes.get(i) as *mut JsRouteDecl
        converter.emit_route_server(name, r, def, inherited)
    }
}

// True when a JSX subtree is purely static: text and native elements whose
// attributes are literals only, with no components, expressions or spreads. Any
// uncertainty makes it dynamic (conservative, never a stale snapshot).
func router_jsx_is_static(node : *mut JsNode) : bool {
    if(node == null) return true
    switch(node.kind) {
        JsNodeKind.JSXText => { return true }
        JsNodeKind.JSXElement => {
            const el = node as *mut JsJSXElement
            if(el.componentSignature != null) return false
            for(var i : uint = 0; i < el.opening.attributes.size(); i++) {
                const attr = el.opening.attributes.get(i)
                if(attr == null) continue
                if(attr.kind != JsNodeKind.JSXAttribute) return false
                const a = attr as *mut JsJSXAttribute
                if(a.value != null && a.value.kind != JsNodeKind.Literal) return false
            }
            for(var i : uint = 0; i < el.children.size(); i++) {
                if(!router_jsx_is_static(el.children.get(i))) return false
            }
            return true
        }
        JsNodeKind.JSXFragment => {
            const f = node as *mut JsJSXFragment
            for(var i : uint = 0; i < f.children.size(); i++) {
                if(!router_jsx_is_static(f.children.get(i))) return false
            }
            return true
        }
        default => { return false }
    }
}

// Phase 7: a route is snapshot-cacheable only when its body is a purely-static
// native subtree (no URL params, no remote, no hooks, no components). This is
// the conservative half of the static/dynamic split — anything uncertain is
// treated as dynamic.
func router_body_is_static(route : *mut JsRouteDecl) : bool {
    if(route.is_url) return false
    if(route.mode.equals(std::string_view("remote"))) return false
    if(route.hooks.size() > 0) return false
    // A layout with nested routes always emits the nested wrappers at (or after)
    // the outlet, which the snapshot of `root` would omit.
    if(router_route_nested(route).size() > 0) return false
    const root = router_route_root(route)
    if(root == null || root.kind != JsNodeKind.JSXElement) return false
    if((root as *mut JsJSXElement).componentSignature != null) return false
    return router_jsx_is_static(root)
}

func (converter : &mut JsConverter) emit_route_server(routerName : std::string_view, route : *mut JsRouteDecl,
                                                      defaultId : std::string_view, inheritedParams : std::string_view) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()

    // Accumulated ancestor patterns: nested id routes under a URL ancestor still
    // receive the ancestor's `{param}` values.
    var effectivePattern = std::string()
    effectivePattern.append_view(&inheritedParams)
    effectivePattern.append_view(&route.pattern)

    // Server-only `<title>` for the selected route (§13.2.1), emitted here (not
    // just at the top level) so a selected nested route contributes its title.
    if(route.title.size() > 0) {
        var titleCall = converter.router_page_stmt(std::string_view("emit_route_title"))
        titleCall.get_args().push(converter.router_string_value(routerName))
        titleCall.get_args().push(converter.router_string_value(route.id))
        titleCall.get_args().push(converter.router_string_value(defaultId))
        titleCall.get_args().push(converter.router_string_value(route.title))
        converter.vec.push(titleCall)
    }

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
    var childrenRefName = std::string()
    // A route with nested children is a layout; its client interactivity is a
    // generated wrapper (native root) or the root component itself with the
    // outlet passed through children (component root). The nested level remains
    // independently hydrated, which is the layout-preservation property.
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
        if(router_body_is_static(route)) {
            // Phase 7: render once, cache the bytes, append on later requests.
            var snapKey = std::string()
            snapKey.append_view(&routerName)
            snapKey.append('#')
            snapKey.append_view(&route.id)
            // Disambiguate identical router names across modules: the route's
            // encoded source location is stable and unique per declaration.
            snapKey.append('@')
            snapKey.append_uinteger(route.decl_loc)
            var appendCall = converter.router_page_value(std::string_view("route_snapshot_append"))
            appendCall.get_args().push(converter.router_string_value(snapKey.to_view()))
            var notAppend = builder.make_not_value(appendCall as *mut Value, location)
            var cacheIf = builder.make_if_stmt(notAppend, converter.parent, location)
            const cacheBody = cacheIf.get_body()
            const savedCacheVec = converter.vec
            converter.vec = cacheBody
            var startNameStr = std::string("snap_")
            startNameStr.append_uinteger(route.decl_loc)
            const startName = builder.allocate_view(startNameStr.to_view())
            var startCall = converter.router_page_value(std::string_view("get_html_size"))
            var startVar = builder.make_varinit_stmt(false, false, &startName, builder.get_u64_type(), startCall as *mut Value, AccessSpecifier.Internal, converter.parent, location)
            converter.vec.push(startVar)
            converter.convertJsNode(root)
            var storeCall = converter.router_page_stmt(std::string_view("route_snapshot_store"))
            storeCall.get_args().push(converter.router_string_value(snapKey.to_view()))
            storeCall.get_args().push(builder.make_identifier(&startName, startVar, false, location))
            converter.vec.push(storeCall)
            converter.vec = savedCacheVec
            converter.vec.push(cacheIf)
        } else {
        // Set the outlet context across the body conversion (save/restore for
        // recursive nesting). Copy the routes vector; do not move the field.
        var prevRoutes = std::vector<*mut JsNode>()
        for(var pi : uint = 0; pi < converter.router_outlet_routes.size(); pi++) {
            prevRoutes.push(converter.router_outlet_routes.get(pi))
        }
        const prevName = converter.router_outlet_name
        const prevDefault = converter.router_outlet_default
        const prevInherited = converter.router_outlet_inherited
        const prevEmitted = converter.router_outlet_emitted
        if(nested.size() > 0) {
            converter.router_outlet_routes = std::vector<*mut JsNode>()
            for(var ni : uint = 0; ni < nested.size(); ni++) {
                converter.router_outlet_routes.push(nested.get(ni))
            }
            converter.router_outlet_name = builder.allocate_view(nestedName.to_view())
            converter.router_outlet_default = builder.allocate_view(&nestedDefault)
            converter.router_outlet_inherited = builder.allocate_view(effectivePattern.to_view())
            converter.router_outlet_emitted = false

            // Hydrated layout: a native-rooted layout gets an anonymous client
            // function so its own markup is interactive. A component-rooted
            // layout that forwards `{props.children}` gets its children passed
            // through `children` so the outlet boundary hydrates too. Either
            // way `<Outlet />` is an opaque boundary that hydration consumes,
            // leaving the SSR'd nested wrappers to the nested router.
            if(root != null && root.kind == JsNodeKind.JSXElement) {
                const rootEl = root as *mut JsJSXElement
                if(rootEl.componentSignature == null) {
                    compName = converter.emit_route_layout_client(route, root)
                } else {
                    // Hydrate the layout component. If it takes explicit
                    // children, pass them through (an inline `<Outlet/>` child
                    // expands to the outlet boundary); if the `<Outlet/>` lives
                    // in the component's own body it renders a slot the runtime
                    // relocates the nested wrappers into.
                    compName = router_route_comp(root)
                    if(rootEl.children.size() > 0) {
                        childrenRefName = converter.emit_route_children_client(route, root)
                    }
                }
            }
        }
        converter.router_inject_param_props(effectivePattern.to_view(), root)
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
        converter.router_outlet_inherited = prevInherited
        converter.router_outlet_emitted = prevEmitted
        }
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
    var baseFirst = true
    // Compile-time attributes declared on a route root component (D-2.7) — e.g.
    // `<Project title="x"/>` — are the component's initial props, so they must be
    // in `baseProps` alongside the param placeholders (otherwise SSR and the
    // client mount disagree). Param-named attributes are skipped: the router
    // injects those server-side, and the `{param}` placeholders + matcher supply
    // them on the client.
    if(root != null && root.kind == JsNodeKind.JSXElement && compName.size() > 0) {
        const rootEl = root as *mut JsJSXElement
        if(rootEl.componentSignature != null) {
            const paramNames = router_pattern_param_names(effectivePattern.to_view())
            var resolved = converter.resolve_attributes(rootEl)
            var filtered = std::vector<ResolvedAttr>()
            for(var ri : uint = 0; ri < resolved.size(); ri++) {
                const a = resolved.get_ptr(ri)
                var isParam = false
                if(a.original != null) {
                    for(var pi : uint = 0; pi < paramNames.size(); pi++) {
                        if(a.original.name.equals(&paramNames.get(pi))) { isParam = true }
                    }
                }
                if(!isParam) { filtered.push(*a) }
            }
            converter.emit_js_props_from_resolved(&filtered, &mut baseFirst)
        }
    }
    router_pattern_params_js(effectivePattern.to_view(), stub, baseFirst)
    stub.append_view("}")
    if(childrenRefName.size() > 0) {
        const childrenView = childrenRefName.to_view()
        stub.append_view(", children: ")
        stub.append_view(&childrenView)
        stub.append_view("()")
    }
    stub.append_view(", wrapperId: \"r")
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
