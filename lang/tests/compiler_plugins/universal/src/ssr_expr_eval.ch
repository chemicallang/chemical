// SSR attribute expression evaluation tests.
// Props-driven expressions in attributes (ternary, ||, &&, comparisons, concat)
// must be evaluated at SSR time so HTML matches the JS client output.

#universal SsrExprButton(props) {
    return <button class={props.variant === "primary" ? "chx-btn chx-btn-primary" : "chx-btn chx-btn-default"} type="button">{props.children}</button>
}

@test
public func universal_ssr_expr_ternary_primary(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="primary" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn chx-btn-primary" type="button"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_ternary_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="outline" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn chx-btn-default" type="button"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprOrButton(props) {
    return <button class={props.primary ? "chx-btn-primary" : "chx-btn-default"}>{props.children}</button>
}

@test
public func universal_ssr_expr_or_truthy(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprOrButton primary={true} /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn-primary"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_or_falsy(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprOrButton primary={false} /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn-default"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprAndButton(props) {
    return <button class={props.active && "chx-btn-active"}>{props.children}</button>
}

@test
public func universal_ssr_expr_and_active(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprAndButton active={true} /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn-active"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_and_inactive(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprAndButton active={false} /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprCompareButton(props) {
    return <button disabled={props.variant === "primary"}>{props.children}</button>
}

@test
public func universal_ssr_expr_compare_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprCompareButton variant="primary" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button disabled="true"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_compare_attr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprCompareButton variant="secondary" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprConcatButton(props) {
    return <button class={"chx-btn chx-btn-" + props.variant + " chx-btn-" + props.size}>{props.children}</button>
}

@test
public func universal_ssr_expr_concat(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprConcatButton variant="primary" size="sm" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><button class="chx-btn chx-btn-primary chx-btn-sm"></button></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprClassName(props) {
    return <div class="chx-card" class={props.className}>{props.children}</div>
}

@test
public func universal_ssr_expr_dual_class(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprClassName className="my-card" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><div class="chx-card my-card"></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// JS-side parity: the client bundle must contain the same expressions, valid JS.
@test
public func universal_ssr_expr_js_parity(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="primary" /> }
    var js = page.getJs()
    if(js.contains("chx-btn-primary")) {
        env.success("ternary class expression present in JS bundle")
    } else {
        env.error("ternary class expression missing from JS bundle")
        env.info(js.data())
    }
}

// Duplicate class attributes must be merged on the JS side (space-joined, with
// reactive unwrapping) to match the SSR HTML's class merging.
@test
public func universal_ssr_expr_js_class_merge(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprClassName className="my-card" /> }
    var js = page.getJs()
    var merged = js.contains("$__uni_value(w))") && js.contains(".filter(Boolean).join")
    var noDupKeys = !js.contains("\"class\": \"chx-card\", \"class\"")
    if(merged && noDupKeys) {
        env.success("duplicate class attrs merged reactively on JS side")
    } else {
        env.error("duplicate class attrs not merged on JS side")
        env.info(js.data())
    }
}

// A spread + duplicate class combination must still merge through $_um.
#universal SsrExprSpreadClass(props) {
    return <button {...props} class="chx-btn" class={props.className}>x</button>
}

@test
public func universal_ssr_expr_spread_class_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprSpreadClass className="extra" variant="primary" /> }
    var js = page.getJs()
    var html = page.getHtml()
    if(js.contains("$_um") && html.contains("chx-btn extra")) {
        env.success("spread + class merge works")
    } else {
        env.error("spread + class merge broken")
        env.info(html.data())
        env.info(js.data())
    }
}

// --- Unified SSR evaluator parity -------------------------------------------------
// These pin the behavior that attribute booleans, attribute values, children, and
// conditions now share one evaluator (eval_ssr_js_expr). Before the unification,
// a static expression could evaluate in one context but return nothing / the wrong
// value in another.

// Static arithmetic in an attribute value must render the numeric result ("3"),
// not a text concatenation of its operands ("12").
#universal SsrStaticArithmeticAttr(props) {
    return <div data-n={1 + 2}></div>
}

@test
public func universal_ssr_static_arithmetic_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStaticArithmeticAttr /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><div data-n="3"></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// A static comparison used as an attribute boolean condition must evaluate
// ("yes"), instead of comparing a concatenated "11" against "2" ("no").
#universal SsrStaticConditionAttr(props) {
    return <div class={1 + 1 === 2 ? "yes" : "no"}></div>
}

@test
public func universal_ssr_static_condition_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStaticConditionAttr /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><div class="yes"></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// A computed body local derived from static state (`var m = n + 1`) must render
// its folded value, exactly like the client's first render.
#universal SsrStaticComputedLocal(props) {
    state n = 5
    var m = n + 1
    return <div>{m}</div>
}

@test
public func universal_ssr_static_computed_local(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStaticComputedLocal /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><div>6</div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// The same static comparison in a body `if` condition (which goes through the
// formerly-separate condition evaluator) must also evaluate.
#universal SsrStaticIfCondition(props) {
    if(1 + 1 === 2) {
        return <span>A</span>
    }
    return <span>B</span>
}

@test
public func universal_ssr_static_if_condition(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStaticIfCondition /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><span>A</span></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// --- Runtime arrays passed as props ----------------------------------------------
// A parent's state array passed as a prop must be serialized so the child can
// map it at SSR. Scalar elements and object elements (resolved via
// ssrAttrValueProp) must both render before JS.

#universal SsrPropScalarChild(props) {
    return <ul data-k="scalar">{props.items.map((it) => <li>{it}</li>)}</ul>
}

#universal SsrPropObjectChild(props) {
    return <ul data-k="object">{props.items.map((it) => <li>{it.text}</li>)}</ul>
}

#universal SsrPropArrayParent(props) {
    state items = ["Apple", "Banana"]
    state objs = [{id: "a", text: "Alpha"}, {id: "b", text: "Beta"}]
    return <div><SsrPropScalarChild items={items} /><SsrPropObjectChild items={objs} /></div>
}

@test
public func universal_ssr_prop_scalar_array_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrPropArrayParent /> }
    var html = page.getHtml()
    if(html.contains("<li>Apple</li>") && html.contains("<li>Banana</li>")) {
        env.success("scalar state array prop maps at SSR")
    } else {
        env.error("scalar state array prop did not render at SSR")
        env.info(html.data())
    }
}

@test
public func universal_ssr_prop_object_array_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrPropArrayParent /> }
    var html = page.getHtml()
    if(html.contains("<li>Alpha</li>") && html.contains("<li>Beta</li>")) {
        env.success("object state array prop resolves item.prop at SSR")
    } else {
        env.error("object state array prop did not render at SSR")
        env.info(html.data())
    }
}

// --- Runtime `.filter()` over props -------------------------------------------------
// `var visible = props.items.filter(it => it.text.includes(props.query))` then
// `{visible.map(...)}` must evaluate the predicate at SSR, not render empty.

#universal SsrFilterChild(props) {
    var visible = props.items.filter((it) => it.text.includes(props.query))
    return <ul data-k="filter">{visible.map((it) => <li>{it.text}</li>)}</ul>
}

#universal SsrFilterParent(props) {
    state items = [{id: "a", text: "Apple"}, {id: "b", text: "Banana"}]
    state query = "an"
    return <div><SsrFilterChild items={items} query={query} /></div>
}

@test
public func universal_ssr_props_filter_runtime_predicate(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrFilterParent /> }
    var html = page.getHtml()
    if(html.contains("<li>Banana</li>") && !html.contains("<li>Apple</li>")) {
        env.success("props-derived .filter() predicate evaluates at SSR")
    } else {
        env.error("props-derived .filter() did not filter at SSR")
        env.info(html.data())
    }
}

// Inline chained form `props.items.filter(pred).map(cb)`.
#universal SsrFilterInline(props) {
    return <ul data-k="filter-inline">{props.items.filter((it) => it.includes("an")).map((it) => <li>{it}</li>)}</ul>
}

@test
public func universal_ssr_props_filter_inline_chain(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrFilterInline items={["Apple", "Banana"]} /> }
    var html = page.getHtml()
    // `items` is an inline string array literal; the filter predicate is applied
    // to it at SSR runtime.
    if(html.contains("<li>Banana</li>") && !html.contains("<li>Apple</li>")) {
        env.success("inline filter+map chain renders at SSR")
    } else {
        env.error("inline filter+map chain did not render at SSR")
        env.info(html.data())
    }
}

// --- Runtime `.filter().length` ------------------------------------------------------
// `{visible.length}` over a props-derived filtered local must count matches at SSR.

#universal SsrFilterCountChild(props) {
    var visible = props.items.filter((it) => it.text.includes(props.query))
    return <p data-k="count">{visible.length}</p>
}

#universal SsrFilterCountParent(props) {
    state items = [{id: "a", text: "Apple"}, {id: "b", text: "Banana"}, {id: "c", text: "Cherry"}]
    state query = "an"
    return <div><SsrFilterCountChild items={items} query={query} /></div>
}

@test
public func universal_ssr_props_filter_length(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrFilterCountParent /> }
    var html = page.getHtml()
    if(html.contains(">1<") && !html.contains(">3<")) {
        env.success("props-derived .filter().length counts at SSR")
    } else {
        env.error("props-derived .filter().length did not count at SSR")
        env.info(html.data())
    }
}

// --- Runtime `.filter()` with toLowerCase() ------------------------------------------
// Case-insensitive predicate over runtime props (mirrors the static evaluator).

#universal SsrFilterFoldChild(props) {
    var visible = props.items.filter((it) => it.toLowerCase().includes(props.query.toLowerCase()))
    return <ul data-k="fold">{visible.map((it) => <li>{it}</li>)}</ul>
}

#universal SsrFilterFoldParent(props) {
    state items = ["Apple", "Banana", "Cherry"]
    state query = "AN"
    return <div><SsrFilterFoldChild items={items} query={query} /></div>
}

@test
public func universal_ssr_props_filter_tolowercase(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrFilterFoldParent /> }
    var html = page.getHtml()
    if(html.contains("<li>Banana</li>") && !html.contains("<li>Apple</li>") && !html.contains("<li>Cherry</li>")) {
        env.success("case-insensitive props .filter() evaluates at SSR")
    } else {
        env.error("case-insensitive props .filter() did not filter at SSR")
        env.info(html.data())
    }
}

// --- Inline object-array literal props ----------------------------------------------
// `items={[{id, text}, ...]}` must serialize (previously failed to link).

#universal SsrInlineObjChild(props) {
    return <ul data-k="inlineobj">{props.items.map((it) => <li>{it.id}:{it.text}</li>)}</ul>
}

#universal SsrInlineObjParent(props) {
    return <div><SsrInlineObjChild items={[{id: "a", text: "Apple"}, {id: "b", text: "Banana"}]} /></div>
}

@test
public func universal_ssr_inline_object_array_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrInlineObjParent /> }
    var html = page.getHtml()
    if(html.contains("<li>a:Apple</li>") && html.contains("<li>b:Banana</li>")) {
        env.success("inline object-array prop maps at SSR")
    } else {
        env.error("inline object-array prop did not render at SSR")
        env.info(html.data())
    }
}

// Components inside a statically-unrolled `.map()` must receive their props at
// SSR, resolved from the bound object element. Regression: the component's
// `item.<prop>` attribute expressions were dropped, so list rows rendered with
// empty props in the server HTML and only appeared correct after hydration.
#universal SsrMapRow(props) {
    return <li data-id={props.id} data-n={props.n}>{props.label}</li>
}

#universal SsrMapList(props) {
    state items = [{id: "a", label: "Alpha", n: 1}, {id: "b", label: "Beta", n: 2}]
    return <ul>{items.map(item => <SsrMapRow key={item.id} id={item.id} n={item.n} label={item.label} />)}</ul>
}

@test
public func universal_ssr_component_props_in_map(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapList /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><ul><li data-id="a" data-n="1">Alpha</li><li data-id="b" data-n="2">Beta</li></ul></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// `state obj = {..}` property reads must resolve at SSR for both children and
// attributes. Regression: they rendered empty server-side (only the attribute
// or text placeholder) and only appeared after hydration.
#universal SsrStateObjComp(props) {
    state user = {name: "Ada", role: "eng", active: true}
    return <div class="user"><span class="n">{user.name}</span><span class="r" data-role={user.role} data-active={user.active}></span></div>
}

@test
public func universal_ssr_state_object_property_reads(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStateObjComp /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}" data-chx-i><div class="user"><span class="n">Ada</span><span class="r" data-role="eng" data-active="true"></span></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}
